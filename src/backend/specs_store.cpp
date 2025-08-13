#include "specs_store.h"
#include <QFile>
#include <QJsonDocument>

SpecsStore::SpecsStore(const QString& path): path_(path){}

bool SpecsStore::load(){
  QFile f(path_);
  if (!f.exists()) return false;
  if (!f.open(QIODevice::ReadOnly)) return false;
  auto doc = QJsonDocument::fromJson(f.readAll());
  if (!doc.isObject()) return false;
  all_ = doc.object();
  return true;
}

bool SpecsStore::save() const{
  QFile f(path_);
  if (!f.open(QIODevice::WriteOnly)) return false;
  QJsonDocument d(all_);
  f.write(d.toJson(QJsonDocument::Indented));
  return true;
}

QJsonObject SpecsStore::get(const QString& id) const{
  auto v = all_.value(id);
  return v.isObject()? v.toObject() : QJsonObject{};
}

void SpecsStore::put(const QString& id, const QJsonObject& spec){
  QJsonObject tmp = all_;
  tmp.insert(id, spec);
  all_ = tmp;
}
