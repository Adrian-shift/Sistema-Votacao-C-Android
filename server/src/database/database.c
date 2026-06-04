#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "database.h"

sqlite3 *db;

int init_database()
{
    if(sqlite3_open("db/votos.db", &db))
    {
        return 0;
    }

    char *err_msg = 0;

    const char *sql_votes =
        "CREATE TABLE IF NOT EXISTS votos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "voter_id TEXT UNIQUE,"
        "candidate TEXT,"
        "receipt TEXT"
        ");";

    sqlite3_exec(
        db,
        sql_votes,
        0,
        0,
        &err_msg
    );

    const char *sql_voters =
        "CREATE TABLE IF NOT EXISTS eleitores ("
        "id TEXT PRIMARY KEY"
        ");";

    sqlite3_exec(
        db,
        sql_voters,
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('101');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('102');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('103');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('104');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('105');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('106');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('107');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('108');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('109');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('110');",
        0,
        0,
        &err_msg
    );

    sqlite3_exec(
        db,
        "INSERT OR IGNORE INTO eleitores VALUES ('20231IREINFINT0040');",
        0,
        0,
        &err_msg
    );

    return 1;
}

int voter_exists(const char *voter_id)
{
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT id FROM eleitores WHERE id=?";

    sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        NULL
    );

    sqlite3_bind_text(
        stmt,
        1,
        voter_id,
        -1,
        SQLITE_STATIC
    );

    int result =
        sqlite3_step(stmt) == SQLITE_ROW;

    sqlite3_finalize(stmt);

    return result;
}

int has_voted(const char *voter_id)
{
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT voter_id FROM votos WHERE voter_id=?";

    sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        NULL
    );

    sqlite3_bind_text(
        stmt,
        1,
        voter_id,
        -1,
        SQLITE_STATIC
    );

    int result =
        sqlite3_step(stmt) == SQLITE_ROW;

    sqlite3_finalize(stmt);

    return result;
}

int count_votes()
{
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT COUNT(*) FROM votos";

    if(sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        NULL
    ) != SQLITE_OK)
    {
        return 0;
    }

    int total = 0;

    if(sqlite3_step(stmt) == SQLITE_ROW)
    {
        total = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return total;
}

int get_vote_results(
    char results[][128],
    int counts[],
    int max_results
)
{
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT candidate, COUNT(*) "
        "FROM votos "
        "GROUP BY candidate "
        "ORDER BY COUNT(*) DESC, candidate ASC";

    if(sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        NULL
    ) != SQLITE_OK)
    {
        return 0;
    }

    int index = 0;

    while(
        sqlite3_step(stmt) == SQLITE_ROW &&
        index < max_results
    )
    {
        const unsigned char *candidate =
            sqlite3_column_text(stmt, 0);

        int count =
            sqlite3_column_int(stmt, 1);

        snprintf(
            results[index],
            128,
            "%s",
            candidate ? (const char*)candidate : "desconhecido"
        );

        counts[index] = count;
        index++;
    }

    sqlite3_finalize(stmt);

    return index;
}

int save_vote(
    const char *voter_id,
    const char *candidate,
    const char *receipt
)
{
    sqlite3_stmt *stmt;

    const char *sql =
        "INSERT INTO votos(voter_id,candidate,receipt)"
        "VALUES(?,?,?)";

    sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        NULL
    );

    sqlite3_bind_text(
        stmt,
        1,
        voter_id,
        -1,
        SQLITE_STATIC
    );

    sqlite3_bind_text(
        stmt,
        2,
        candidate,
        -1,
        SQLITE_STATIC
    );

    sqlite3_bind_text(
        stmt,
        3,
        receipt,
        -1,
        SQLITE_STATIC
    );

    int result =
        sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);

    return result;
}
