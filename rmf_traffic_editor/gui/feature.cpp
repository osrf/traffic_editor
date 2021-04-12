/*
 * Copyright (C) 2021 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <cmath>

#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QString>

#include "feature.hpp"


Feature::Feature()
{
  _uuid = QUuid::createUuid();
}

Feature::Feature(double x, double y, int id)
: _x(x), _y(y), _id(id)
{
  _uuid = QUuid::createUuid();
}

void Feature::from_yaml(const YAML::Node& data)
{
  if (!data.IsMap())
    throw std::runtime_error("Feature::from_yaml() expected a map");
  _x = data["x"].as<double>();
  _y = data["y"].as<double>();
  _id = data["id"].as<uint16_t>();
  _uuid = QString(data["uuid"].as<std::string>().c_str());
}

YAML::Node Feature::to_yaml() const
{
  YAML::Node node;
  node.SetStyle(YAML::EmitterStyle::Flow);
  node["x"] = std::round(_x * 1000.0) / 1000.0;
  node["y"] = std::round(_y * 1000.0) / 1000.0;
  node["id"] = _id;
  node["uuid"] = _uuid.toString().toStdString();
  return node;
}

void Feature::draw(
  QGraphicsScene* scene,
  double meters_per_pixel) const
{
  // todo: choose color based on layer color
  const double a = 0.5;
  const QColor color = QColor::fromRgbF(0.0, 0.0, 1.0, a);
  const QColor selected_color = QColor::fromRgbF(1.0, 0.0, 0.0, a);

  QPen pen(_selected ? selected_color : color);
  pen.setWidth(0.05 / meters_per_pixel);
  const double radius = 0.25 / meters_per_pixel;

  QGraphicsLineItem *vertical_line = scene->addLine(
    _x,
    _y - radius,
    _x,
    _y + radius,
    pen);
  vertical_line->setZValue(20.0);

  QGraphicsLineItem *horizontal_line = scene->addLine(
    _x - radius,
    _y,
    _x + radius,
    _y,
    pen);
  horizontal_line->setZValue(20.0);

  QGraphicsEllipseItem* circle = scene->addEllipse(
    _x - radius,
    _y - radius,
    2 * radius,
    2 * radius,
    pen,
    QBrush());  // default brush is transparent
  circle->setZValue(20.0);

  QGraphicsSimpleTextItem* item = scene->addSimpleText(QString::number(_id));
  item->setBrush(QColor(0, 0, 255, 255));
  item->setPos(_x, _y + radius);
  auto font = item->font();
  font.setPointSize(30);
  item->setFont(font);
}