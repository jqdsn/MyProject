#include <gtest/gtest.h>
#include <QtCore>
#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>

static QByteArray http(const QString& method, const QUrl& url, const QByteArray& body=QByteArray()){
  QTcpSocket s;
  s.connectToHost(url.host(), url.port(8080));
  if (!s.waitForConnected(3000)) return {};
  QByteArray req;
  req += method.toUtf8() + " " + url.path().toUtf8() + " HTTP/1.1\r\n";
  req += "Host: " + url.host().toUtf8() + "\r\n";
  if (!body.isEmpty()){
    req += "Content-Type: application/json\r\n";
    req += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
  }
  req += "Connection: close\r\n\r\n";
  req += body;
  s.write(req); s.flush();
  if (!s.waitForReadyRead(5000)) return {};
  QByteArray resp; while (s.waitForReadyRead(1000)) resp += s.readAll();
  if (resp.isEmpty()) resp = s.readAll();
  auto pos = resp.indexOf("\r\n\r\n");
  return pos>=0? resp.mid(pos+4) : resp;
}

TEST(HttpE2E, MeasureParallelLinesUsingDefaultSpec){
  // Start backend process
  QProcess p;
  p.setProgram(QCoreApplication::applicationDirPath()+"/../src/Release/myproject_backend.exe");
  p.setProcessChannelMode(QProcess::MergedChannels);
  p.start(); ASSERT_TRUE(p.waitForStarted(3000));
  QThread::msleep(800); // give it a moment to bind

  // Health
  auto health = http("GET", QUrl("http://localhost:8080/health"));
  ASSERT_FALSE(health.isEmpty());

  // POST /measure with image_path and spec_id=default
  QJsonObject payload;
  payload["image_path"] = "tests/data/parallel_lines.png";
  payload["spec_id"] = "default";
  payload["roi"] = QJsonObject{{"type","rect"},{"x",50},{"y",100},{"w",500},{"h",200}};
  QJsonDocument d(payload);

  auto bytes = http("POST", QUrl("http://localhost:8080/measure"), d.toJson(QJsonDocument::Compact));
  ASSERT_FALSE(bytes.isEmpty());

  auto doc = QJsonDocument::fromJson(bytes);
  ASSERT_TRUE(doc.isObject());
  auto arr = doc.object().value("metrics").toArray();
  ASSERT_FALSE(arr.isEmpty());

  // Expect at least line_gap and parallelism
  bool hasGap=false, hasPar=false;
  for (auto v: arr){
    auto m = v.toObject();
    auto name = m.value("name").toString();
    if (name=="line_gap"){ hasGap=true; }
    if (name=="parallelism"){ hasPar=true; }
  }
  EXPECT_TRUE(hasGap);
  EXPECT_TRUE(hasPar);

  p.kill();
  p.waitForFinished();
}

