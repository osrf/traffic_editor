#include <functional>
#include <gazebo/common/Plugin.hh>
#include <gazebo/gui/GuiPlugin.hh>
#include <gazebo/transport/transport.hh>

#include <gazebo/msgs/msgs.hh>
#include <gazebo/rendering/rendering.hh>

#include <string>
using std::string;


class ToggleFloors : public gazebo::GUIPlugin
{
  Q_OBJECT
  gazebo::transport::NodePtr node;
  gazebo::transport::PublisherPtr visual_pub;

public:
  ToggleFloors()
  : GUIPlugin()
  {
    printf("ToggleFloors::ToggleFloors()\n");
    node = gazebo::transport::NodePtr(new gazebo::transport::Node());
    node->Init();
    visual_pub = node->Advertise<gazebo::msgs::Visual>("~/visual");
  }

  virtual ~ToggleFloors()
  {
  }

  void Load(sdf::ElementPtr sdf)
  {
    printf("ToggleFloors::Load()\n");

    QHBoxLayout* hbox = new QHBoxLayout;

    for (sdf::ElementPtr floor_ele = sdf->GetFirstElement();
      floor_ele;
      floor_ele = floor_ele->GetNextElement("floor"))
    {
      if (floor_ele->GetName() != string("floor"))
        continue;
      string floor_name = floor_ele->GetAttribute("name")->GetAsString();
      string model_name =
        floor_ele->GetAttribute("model_name")->GetAsString();

      std::vector<string> models;
      auto model_ele = floor_ele->GetElement("model");
      while (model_ele)
      {
        if (model_ele->HasAttribute("name"))
          models.push_back(model_ele->GetAttribute("name")->GetAsString());
        model_ele = model_ele->GetNextElement("model");
      }

      printf(
        "ToggleFloors::Load found a floor element: [%s]->[%s]\n",
        floor_name.c_str(),
        model_name.c_str());

      QPushButton* button =
        new QPushButton(QString::fromStdString(floor_name));
      button->setCheckable(true);
      button->setChecked(true);
      connect(
        button,
        &QAbstractButton::clicked,
        [this, button, model_name, models]()
        {
          this->button_clicked(button, model_name, models);
        });
      hbox->addWidget(button);
    }
    setLayout(hbox);
  }

  void button_clicked(
    QPushButton* button, string model_name, std::vector<string> models)
  {
    bool visible = button->isChecked();
    printf(
      "clicked: [%s] %s\n",
      model_name.c_str(),
      visible ? "SHOW" : "HIDE");
    gazebo::msgs::Visual visual_msg;
    visual_msg.set_parent_name("world");
    visual_msg.set_name(model_name);
    visual_msg.set_visible(visible);
    visual_pub->Publish(visual_msg);
    for (const string& model : models)
    {
      visual_msg.set_name(model);
      visual_pub->Publish(visual_msg);
    }
  }
};

#include "toggle_floors.moc"

GZ_REGISTER_GUI_PLUGIN(ToggleFloors)
