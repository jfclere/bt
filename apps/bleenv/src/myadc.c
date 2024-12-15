#include "syscfg/syscfg.h"
#include "sysinit/sysinit.h"
#include <os/os.h>
#if MYNEWT_VAL(ADC_0) == 1

#include <bsp/bsp.h>
#include <adc/adc.h>
#include <adc_nrf52/adc_nrf52.h>
#include <nrf_saadc.h>
#include <console/console.h>

#include <string.h>

#include "myadc.h"

#define ADC_NUMBER_SAMPLES (1)
#define ADC_NUMBER_CHANNELS (1)
#define ADC_CHANNEL_0 (0)

/* /\* ADC Task settings *\/ */
#define ADC_TASK_PRIO           (254)
#define ADC_STACK_SIZE          (OS_STACK_ALIGN(128))
static struct os_task adc_task;
static os_stack_t adc_stack[ADC_STACK_SIZE];

static uint8_t *sample_buffer1;
static uint8_t *sample_buffer2;

static uint16_t result_mv;

uint16_t get_result_mv()
{
    return result_mv;
}


static int
adc_read(struct adc_dev *dev, void *buffer, int buffer_len)
{
    int i;
    int adc_result;
    int rc;

    for (i = 0; i < ADC_NUMBER_SAMPLES; i++) {
        rc = adc_buf_read(dev, buffer, buffer_len, i, &adc_result);
        if (rc != 0) {
            goto err;
        }
        printf("read: %d\n", adc_result);
        result_mv = adc_result_mv(dev, 0, adc_result);
    }
    adc_buf_release(dev, buffer, buffer_len);
    
    return result_mv;
err:
    return (rc);
}

static int
adc_read_event(struct adc_dev *dev,
               void *arg,
               uint8_t etype,
               void *buffer,
               int buffer_len)
{
    int rc = -1;

    switch (etype) {
    case ADC_EVENT_CALIBRATED:
        console_printf("CALIBRATED\n");
        break;
    case ADC_EVENT_RESULT:
        rc = adc_read(dev, buffer, buffer_len);
        if (rc == -1)
            return (rc);
        break;
    default:
        console_printf("Oops not expected %d\n", etype);
        goto err;
    }

    return (0);
err:
    return (rc);
}

static struct adc_dev *adc_init()
{
    struct adc_dev_cfg cfg = {
        .resolution = ADC_RESOLUTION_12BIT,
        .oversample = ADC_OVERSAMPLE_DISABLED,
        .calibrate = false
    };

    struct adc_chan_cfg channel_0_config = {
        .reference = ADC_REFERENCE_INTERNAL,
        .gain = ADC_GAIN1_6,
        .pin = NRF_SAADC_INPUT_AIN7, /* P0.31 / A7 */
        .differential = false,
        .pin_negative = 0
    };

    struct adc_dev *adc = (struct adc_dev *) os_dev_open("adc0",
                                         OS_TIMEOUT_NEVER,
                                         &cfg);
    console_printf("adc_init os_dev_open %p\n", adc);
    assert(adc != NULL);
    adc_chan_config(adc, ADC_CHANNEL_0, &channel_0_config);
    console_printf("adc_init adc_chan_config DONE!!!\n");


    sample_buffer1 = malloc(adc_buf_size(adc,
                                         ADC_NUMBER_CHANNELS,
                                         ADC_NUMBER_SAMPLES));
    sample_buffer2 = malloc(adc_buf_size(adc,
                                         ADC_NUMBER_CHANNELS,
                                         ADC_NUMBER_SAMPLES));

    memset(sample_buffer1, 0, adc_buf_size(adc,
                                           ADC_NUMBER_CHANNELS,
                                           ADC_NUMBER_SAMPLES));
    memset(sample_buffer2, 0, adc_buf_size(adc,
                                           ADC_NUMBER_CHANNELS,
                                           ADC_NUMBER_SAMPLES));
    int rc;
    rc = adc_buf_set(adc,
                sample_buffer1,
                sample_buffer2,
                adc_buf_size(adc, ADC_NUMBER_CHANNELS, ADC_NUMBER_SAMPLES));
    console_printf("adc_init adc_buf_set %d\n", rc);
    assert(rc == 0);
    rc = adc_event_handler_set(adc, adc_read_event, (void *) NULL);
    console_printf("adc_init adc_event_handler_set: %d\n", rc);
    assert(rc == 0);
    return adc;
}

/**
 * Event loop for the sensor task.
 */
static void adc_task_handler(void *unused)
{
    int rc;

    struct adc_dev *adc = adc_init();

    while (1) {
        /* Wait 30 second */
        os_time_delay(OS_TICKS_PER_SEC * 30);

        rc = adc_sample(adc);
        console_printf("adc_task_handler adc_sample %d\n", rc);
        assert(rc == 0);
        if (result_mv != -1) {
            console_printf("Ch Got %d\n", result_mv);
        } else {
            console_printf("Error while reading\n");
        }
    }
}

void start_adc_task()
{
    console_printf("start_adc_tasks\n");
    os_task_init(&adc_task,
                 "sensor",
                 adc_task_handler,
                 NULL,
                 ADC_TASK_PRIO,
                 OS_WAIT_FOREVER,
                 adc_stack,
                 ADC_STACK_SIZE);
}
#else
void start_adc_task()
{
}
uint16_t get_result_mv()
{
    return 0;
}
#endif
