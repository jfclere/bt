#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <string.h>

/* Use postgresql database */

/* createdb bme280 --owner jfclere
 * psql -d bme280
  CREATE TABLE measurements (
  time bigint,
  temp NUMERIC(4,2),
  pres NUMERIC(6,2),
  humi NUMERIC(5,2));
 *
 * bme280=> select * from measurements;
 *     time    | temp  |  pres   | humi
 * ------------+-------+---------+-------
 *  1691049904 | 27.04 | 1008.77 | 51.22
 * (1 row)
 * tested!
 *
 */

static void do_exit(PGconn *conn, PGresult *res) {
    fprintf(stderr, "%s\n", PQerrorMessage(conn));    
    PQclear(res);
    PQfinish(conn);    
    exit(1);
}

void inserttemp(char *table, time_t t, float temp, float pres, float humi) {
    // Here is the database connection being used
    PGconn *conn = PQconnectdb("user=jfclere dbname=bme280");
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    char sinto[100];
    sprintf(sinto, "INSERT INTO %s VALUES($1,$2,$3,$4)", table);
    char p1[100], p2[100], p3[100], p4[100];
    const char *paramValues[] = { p1, p2, p3, p4 };
    sprintf(p1, "%d", t);
    sprintf(p2, "%4.2f", temp);
    sprintf(p3, "%6.2f", pres);
    sprintf(p4, "%4.2f", humi);
    printf("doing %s %s %s %s %s\n", sinto, p1, p2, p3, p4);
    // make call to database server
    PGresult *res = PQexecParams(conn, sinto, 4, NULL, paramValues, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) 
        do_exit(conn, res);     
    PQclear(res);
    PQfinish(conn);
}
