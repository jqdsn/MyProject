#pragma once
#include <QJsonObject>
#include <QString>

class SpecsStore {
public:
  explicit SpecsStore(const QString& path);
  bool load();
  bool save() const;
  QJsonObject get(const QString& id) const;
  void put(const QString& id, const QJsonObject& spec);
private:
  QString path_;
  QJsonObject all_;
};
