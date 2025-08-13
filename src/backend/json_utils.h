#pragma once
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>

inline QByteArray toBytes(const QJsonObject& o){
  QJsonDocument d(o); return d.toJson(QJsonDocument::Compact);
}
