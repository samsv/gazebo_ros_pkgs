// Copyright 2013 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gazebo/common/Events.hh>
#include <gazebo/physics/Link.hh>
#include <gazebo/physics/Model.hh>
#include <gazebo_plugins/gazebo_ros_force.hpp>
#include <gazebo_ros/node.hpp>
#include <rclcpp/rclcpp.hpp>

#include <memory>
#include <string>

namespace gazebo_plugins
{
class GazeboRosForcePrivate
{
public:
  /// A pointer to the Link, where force is applied
  gazebo::physics::LinkPtr link_;

  /// A pointer to the GazeboROS node.
  gazebo_ros::Node::SharedPtr ros_node_;

  /// Wrench subscriber
  rclcpp::Subscription<geometry_msgs::msg::Wrench>::SharedPtr wrench_sub_;

  /// Container for the wrench force that this plugin exerts on the body.
  geometry_msgs::msg::Wrench wrench_msg_;

  // Pointer to the update event connection
  gazebo::event::ConnectionPtr update_connection_;
};

GazeboRosForce::GazeboRosForce()
: impl_(std::make_unique<GazeboRosForcePrivate>())
{
  impl_->wrench_msg_.force.x = 0.0;
  impl_->wrench_msg_.force.y = 0.0;
  impl_->wrench_msg_.force.z = 0.0;
  impl_->wrench_msg_.torque.x = 0.0;
  impl_->wrench_msg_.torque.y = 0.0;
  impl_->wrench_msg_.torque.z = 0.0;
}

GazeboRosForce::~GazeboRosForce()
{
}

void GazeboRosForce::Load(gazebo::physics::ModelPtr model, sdf::ElementPtr sdf)
{
  auto logger = rclcpp::get_logger("gazebo_ros_force");

  // Target link
  if (!sdf->HasElement("link_name")) {
    RCLCPP_ERROR(logger, "Force plugin missing <link_name>, cannot proceed");
    return;
  }

  auto link_name = sdf->GetElement("link_name")->Get<std::string>();

  impl_->link_ = model->GetLink(link_name);
  if (!impl_->link_) {
    RCLCPP_ERROR(logger, "Link named: %s does not exist\n", link_name.c_str());
    return;
  }

  // Subscribe to wrench messages
  impl_->ros_node_ = gazebo_ros::Node::Create("gazebo_ros_force", sdf);

  impl_->wrench_sub_ = impl_->ros_node_->create_subscription<geometry_msgs::msg::Wrench>(
    "gazebo_ros_force", std::bind(&GazeboRosForce::OnRosWrenchMsg, this,
    std::placeholders::_1));

  // Callback on every iteration
  impl_->update_connection_ = gazebo::event::Events::ConnectWorldUpdateBegin(
    std::bind(&GazeboRosForce::OnUpdate, this));
}

void GazeboRosForce::OnRosWrenchMsg(const geometry_msgs::msg::Wrench::SharedPtr msg)
{
  impl_->wrench_msg_.force.x = msg->force.x;
  impl_->wrench_msg_.force.y = msg->force.y;
  impl_->wrench_msg_.force.z = msg->force.z;
  impl_->wrench_msg_.torque.x = msg->torque.x;
  impl_->wrench_msg_.torque.y = msg->torque.y;
  impl_->wrench_msg_.torque.z = msg->torque.z;
}

void GazeboRosForce::OnUpdate()
{
  ignition::math::Vector3d force(impl_->wrench_msg_.force.x,
    impl_->wrench_msg_.force.y,
    impl_->wrench_msg_.force.z);
  ignition::math::Vector3d torque(impl_->wrench_msg_.torque.x,
    impl_->wrench_msg_.torque.y,
    impl_->wrench_msg_.torque.z);
  impl_->link_->AddForce(force);
  impl_->link_->AddTorque(torque);
}

GZ_REGISTER_MODEL_PLUGIN(GazeboRosForce)
}  // namespace gazebo_plugins
