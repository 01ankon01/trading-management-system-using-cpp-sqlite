#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "sqlite/sqlite3.h"
#include <fstream>
#include <sstream>

using namespace std;

class Trade {
protected:
    string symbol;
    int quantity;
    double price;

public:
    Trade(string symbol, int quantity, double price) {
        this->symbol = symbol;
        this->quantity = quantity;
        this->price = price;
    }

    virtual double getCost() {
        return quantity * price;
    }

    virtual void display() {
        cout << "Symbol: " << symbol << endl;
        cout << "Quantity: " << quantity << endl;
        cout << "Price: " << price << endl;
    }
};

class BuyTrade : public Trade {
public:
    BuyTrade(string symbol, int quantity, double price) : Trade(symbol, quantity, price) {}

    double getCost()  {
        return quantity * price * 1.01; // add 1% commission for buy trades
    }

    void display() {
        cout << "Buy Trade Details:" << endl;
        Trade::display();
    }
};

class SellTrade : public Trade {
public:
    SellTrade(string symbol, int quantity, double price) : Trade(symbol, quantity, price) {}

    double getCost()  {
        return quantity * price * 0.99; // deduct 1% commission for sell trades
    }

    void display() {
        cout << "Sell Trade Details:" << endl;
        Trade::display();
    }
};

int main() {
    sqlite3* db;
    int rc = sqlite3_open("trades.db", &db);
    if (rc != SQLITE_OK) {
        cerr << "Error opening database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return 1;
    }

    // create trades table if it does not exist
    char* error_message = NULL;
    const char* sql = "CREATE TABLE IF NOT EXISTS trades ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "symbol TEXT NOT NULL,"
                      "quantity INTEGER NOT NULL,"
                      "price REAL NOT NULL,"
                      "type TEXT NOT NULL,"
                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
                      ");";
    rc = sqlite3_exec(db, sql, NULL, 0, &error_message);
    if (rc != SQLITE_OK) {
        cerr << "Error creating trades table: " << error_message << endl;
        sqlite3_free(error_message);
        sqlite3_close(db);
        return 1;
    }

    vector<Trade*> trades;
    int choice;

    while (true) {
        cout << "1. Add buy trade" << endl;
        cout << "2. Add sell trade" << endl;
        cout << "3. View portfolio" << endl;
        cout << "4. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
            case 1: {
                string symbol;
                int quantity;
                double price;
                cout << "Enter stock symbol: ";
                cin >> symbol;
                cout << "Enter quantity: ";
                cin >> quantity;
                cout << "Enter current price: ";
                cin >> price;
                Trade* buyTrade = new BuyTrade(symbol, quantity, price);
                trades.push_back(buyTrade);

                // add buy trade to database
                char* error_message = NULL;
                const char* sql = "INSERT INTO trades (symbol, quantity, price, type) "
                                  "VALUES (?, ?, ?, 'buy');";
                sqlite3_stmt* stmt;
                rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
                if (rc != SQLITE_OK) {
                    cerr << "Error preparing statement: " << sqlite3_errmsg(db) << endl;
                    sqlite3_close(db);
                    return 1;
                }
                rc = sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
                rc = sqlite3_bind_int(stmt, 2, quantity);
                rc = sqlite3_bind_double(stmt, 3, price);
                rc = sqlite3_step(stmt);
                if (rc != SQLITE_DONE) {
                    cerr << "Error inserting buy trade: " << sqlite3_errmsg(db) << endl;
                    sqlite3_finalize(stmt);
                    sqlite3_close(db);
                    return 1;
                }
                sqlite3_finalize(stmt);

                cout << "Buy trade added successfully" << endl;
                break;
            }
                case 2: {
                    string symbol;
                    int quantity;
                    double price;
                    cout << "Enter stock symbol: ";
                    cin >> symbol;
                    cout << "Enter quantity: ";
                    cin >> quantity;
                    cout << "Enter price: ";
                    cin >> price;
                    Trade* sellTrade = new SellTrade(symbol, quantity, price);
                    trades.push_back(sellTrade);

                    // add sell trade to database
                    char* error_message = NULL;
                    const char* sql = "INSERT INTO trades (symbol, quantity, price, type) "
                                    "VALUES (?, ?, ?, 'sell');";
                    sqlite3_stmt* stmt;
                    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
                    if (rc != SQLITE_OK) {
                        cerr << "Error preparing statement: " << sqlite3_errmsg(db) << endl;
                        sqlite3_close(db);
                        return 1;
                    }
                    rc = sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
                    rc = sqlite3_bind_int(stmt, 2, quantity);
                    rc = sqlite3_bind_double(stmt, 3, price);
                    rc = sqlite3_step(stmt);
                    if (rc != SQLITE_DONE) {
                        cerr << "Error inserting sell trade: " << sqlite3_errmsg(db) << endl;
                        sqlite3_finalize(stmt);
                        sqlite3_close(db);
                        return 1;
                    }
                    sqlite3_finalize(stmt);

                    cout << "Sell trade added successfully" << endl;
                    break;
                }
                    case 3: {
                        // retrieve all trades from database
                        const char* sql = "SELECT * FROM trades;";
                        sqlite3_stmt* stmt;
                        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
                        if (rc != SQLITE_OK) {
                            cerr << "Error preparing statement: " << sqlite3_errmsg(db) << endl;
                            sqlite3_close(db);
                            return 1;
                        }

                        // open file for writing
                        ofstream outfile("portfolio.txt");
                        if (!outfile.is_open()) {
                            cerr << "Error opening portfolio file for writing" << endl;
                            sqlite3_finalize(stmt);
                            sqlite3_close(db);
                            return 1;
                        }

                        // write header row to file
                        outfile << "ID\tSymbol\tQuantity\tPrice\tType\tTimestamp" << endl;

                        // iterate over rows and write to file
                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                            int id = sqlite3_column_int(stmt, 0);
                            const char* symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                            int quantity = sqlite3_column_int(stmt, 2);
                            double price = sqlite3_column_double(stmt, 3);
                            const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                            const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                            outfile << id << "\t" << symbol << "\t" << quantity << "\t" << price << "\t" << type << "\t" << timestamp << endl;
                        }

                        // close file and finalize statement
                        outfile.close();
                        sqlite3_finalize(stmt);

                        cout << "Portfolio written to file: portfolio.txt" << endl;
                        break;
                    }
                    case 4: {
                        sqlite3_close(db);
                        cout << "Exiting..." << endl;
                        return 0;
                    }
                    default: {
                        cout << "Invalid choice" << endl;
                        break;
            }
        }
    }
//    // delete allocated memory
//    for (auto trade : trades) {
//        delete trade;
//    }

    return 0;
}
