#include <QCoreApplication>
#include <QHttpServer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTcpServer>

#include <memory>

using namespace Qt::StringLiterals;
using StatusCode = QHttpServerResponder::StatusCode;

constexpr int port = 53998;
const QString root = u"/api/v1/"_s;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setHostName("Thunder_Note");
    db.setDatabaseName("notes.sdb");
    if (!db.open()) {
        qCritical() << "Can't open DB";
        return -1;
    }

    QSqlQuery query;
    QString createTableQuery =
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "note TEXT NOT NULL, "
        "date TIMESTAMP NOT NULL"
        ");";

    if (!query.exec(createTableQuery)) {
        qCritical() << "Failed to create table:" << query.lastError().text();
        return -1;
    }

    QHttpServer httpServer;

    httpServer.route(root + u"notes"_s, QHttpServerRequest::Method::Get, [](const QHttpServerRequest &request){
        QSqlQuery query("SELECT * FROM notes ORDER BY date DESC");
        QJsonArray response;
        while (query.next()) {
            QJsonObject obj;
            obj.insert("id", query.value(0).toJsonValue());
            obj.insert("note", query.value(1).toJsonValue());
            obj.insert("date", query.value(2).toDateTime().toString(Qt::ISODate));
            response.push_back(obj);
        }
        return QHttpServerResponse{ response };
    });

    httpServer.route(root + u"notes/<arg>"_s, QHttpServerRequest::Method::Get, [](qint64 itemId, const QHttpServerRequest &request){
        QSqlQuery query;
        query.prepare("SELECT * FROM notes WHERE id=:id");
        query.bindValue(":id", itemId);
        QJsonObject response;
        if (!query.exec()) {
            qCritical() << query.lastError().text();
            return QHttpServerResponse{ StatusCode::InternalServerError };
        }
        while (query.next()) {
            response.insert("id", query.value(0).toJsonValue());
            response.insert("note", query.value(1).toJsonValue());
            response.insert("date", query.value(2).toString());
        }
        if (response.isEmpty())
            return QHttpServerResponse{ StatusCode::NotFound };
        return QHttpServerResponse { response };
    });

    httpServer.route(root + u"notes"_s, QHttpServerRequest::Method::Post, [](const QHttpServerRequest &request){
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(request.body(), &err);
        if (err.error || !doc.isObject())
            return QHttpServerResponse{ StatusCode::BadRequest };
        auto json = doc.object();
        auto note = json.value("note").toString("");
        if (note == "") {
            return QHttpServerResponse{ StatusCode::BadRequest };
        }
        QSqlQuery query;
        query.prepare("INSERT INTO notes(note, date) values (:note, :date)");
        query.bindValue(":note", note);
        query.bindValue(":date", QDateTime::currentDateTimeUtc());
        if (!query.exec()) {
            qCritical() << query.lastError().text();
            return QHttpServerResponse{ StatusCode::InternalServerError };
        }
        return QHttpServerResponse{ StatusCode::Ok };
    });

    httpServer.route(root + u"notes/<arg>"_s, QHttpServerRequest::Method::Put, [](qint64 itemId, const QHttpServerRequest &request){
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(request.body(), &err);
        if (err.error || !doc.isObject())
            return QHttpServerResponse{ StatusCode::BadRequest };
        auto json = doc.object();
        auto note = json.value("note").toString("");
        if (note == "") {
            return QHttpServerResponse{ StatusCode::BadRequest };
        }
        QSqlQuery query;
        query.prepare("UPDATE notes SET note=:note, date=:date WHERE id=:id");
        query.bindValue(":id", itemId);
        query.bindValue(":note", note);
        query.bindValue(":date", QDateTime::currentDateTimeUtc());
        if (!query.exec()) {
            qCritical() << query.lastError().text();
            return QHttpServerResponse{ StatusCode::InternalServerError };
        }
        return QHttpServerResponse{ StatusCode::Ok };
    });

    httpServer.route(root + u"notes/<arg>"_s, QHttpServerRequest::Method::Delete, [](qint64 itemId, const QHttpServerRequest &request){
        QSqlQuery query;
        query.prepare("DELETE FROM notes WHERE id=:id");
        query.bindValue(":id", itemId);
        if (!query.exec()) {
            qCritical() << query.lastError().text();
            return QHttpServerResponse{ StatusCode::InternalServerError };
        }
        return QHttpServerResponse{ StatusCode::Ok };
    });

    auto tcpserver = std::make_unique<QTcpServer>();
    if (!tcpserver->listen(QHostAddress::Any, port) || !httpServer.bind(tcpserver.get())) {
        qCritical() << "Server failed to listen on a port: " << port;
        return 0;
    }
    return app.exec();
}
