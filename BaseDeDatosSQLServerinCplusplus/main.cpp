#include <windows.h>
#include <sqlext.h>
#include <iostream>
#include <string>
#include <cwchar>

// Function to convert ASCII (char*) to Unicode (SQLWCHAR*)
SQLWCHAR* convertToSQLWCHAR(const char* str) {
    size_t length = strlen(str) + 1; // Include the null terminator
    SQLWCHAR* wStr = new SQLWCHAR[length]; // Buffer for the converted string
    size_t convertedChars = 0; // Variable to store the number of converted characters

    errno_t err = mbstowcs_s(&convertedChars, wStr, length, str, length - 1);
    if (err != 0) {
        std::cerr << "Error converting string to Unicode." << std::endl;
        delete[] wStr;
        return nullptr;
    }

    return wStr;
}

// Function to execute an SQL statement
bool executeSQL(SQLHANDLE sqlStmtHandle, const char* query) {
    SQLWCHAR* sqlQuery = convertToSQLWCHAR(query);
    if (!sqlQuery) return false;

    SQLRETURN retCode = SQLExecDirect(sqlStmtHandle, sqlQuery, SQL_NTS);
    delete[] sqlQuery;

    if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO) {
        return true;
    }
    else {
        SQLCHAR sqlState[1024];
        SQLCHAR message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRecA(SQL_HANDLE_STMT, sqlStmtHandle, 1, sqlState, &nativeError, message, sizeof(message), &textLength);
        std::cerr << "SQL Error: " << message << " (SQLState: " << sqlState << ")" << std::endl;
        return false;
    }
}

int main() {
    SQLHANDLE sqlEnvHandle = nullptr;
    SQLHANDLE sqlConnHandle = nullptr;
    SQLHANDLE sqlStmtHandle = nullptr;
    SQLRETURN retCode = 0;

    // Initialize ODBC
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlEnvHandle);
    SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, &sqlConnHandle);

    // Connection string converted to Unicode
    const char* connectionStringASCII = "DRIVER={ODBC Driver 17 for SQL Server};SERVER=(localdb)\\MSSQLLocalDB;DATABASE=TestDB;Trusted_Connection=yes;";
    SQLWCHAR* connectionString = convertToSQLWCHAR(connectionStringASCII);

    // Connect to the database
    retCode = SQLDriverConnect(sqlConnHandle, NULL, connectionString, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (retCode != SQL_SUCCESS && retCode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Error connecting to the database." << std::endl;
        delete[] connectionString;
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return -1;
    }
    std::cout << "Connected successfully." << std::endl;

    delete[] connectionString;

    // Allocate a statement handle
    SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, &sqlStmtHandle);

    // Create the Users table
    const char* createTableSQL = R"(
    IF OBJECT_ID('Users', 'U') IS NULL
    BEGIN
        CREATE TABLE Users (
            ID INT PRIMARY KEY IDENTITY(1,1),
            Name NVARCHAR(50),
            Age INT
        );
    END;
)";

    if (!executeSQL(sqlStmtHandle, createTableSQL)) {
        std::cerr << "Error creating table." << std::endl;
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLDisconnect(sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return -1;
    }
    std::cout << "Table 'Users' created successfully." << std::endl;

    // Insert seed data into Users table
    const char* seedDataSQL = R"(
        INSERT INTO Users (Name, Age) VALUES
        ('Alice', 30),
        ('Bob', 25),
        ('Charlie', 35);
    )";

    if (!executeSQL(sqlStmtHandle, seedDataSQL)) {
        std::cerr << "Error inserting seed data." << std::endl;
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLDisconnect(sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        return -1;
    }
    std::cout << "Seed data inserted successfully." << std::endl;

    // Clean up
    SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
    SQLDisconnect(sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
    SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);

    return 0;
}
