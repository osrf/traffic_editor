#include <memory>

#include <building_sim_common/utils.hpp>
#include <building_sim_common/door_common.hpp>

using namespace std::chrono_literals;

namespace building_sim_common {

rclcpp::Logger DoorCommon::logger() const
{
  return rclcpp::get_logger("door_" + _state.door_name);
}

DoorMode DoorCommon::requested_mode() const
{
  return _request.requested_mode;
}

std::vector<std::string> DoorCommon::joint_names() const
{
  std::vector<std::string> joint_names;
  for (const auto& door : _doors)
    joint_names.push_back(door.first);
  
  return joint_names;
}

MotionParams& DoorCommon::params()
{
  return _params;
}

bool DoorCommon::is_initialized() const
{
  return _initialized;
}

void DoorCommon::publish_state(const uint32_t door_value,
  const rclcpp::Time& time)
{
  if (!_initialized)
    return;

  _state.current_mode.value = door_value;
  _state.door_time = time;
  _door_state_pub->publish(_state);
}


DoorCommon::DoorCommon(const std::string& door_name,
  rclcpp::Node::SharedPtr node,
  const MotionParams& params,
  const std::string& left_door_joint_name,
  const std::string& right_door_joint_name,
  const std::array<double, 2>& left_joint_limits,
  const std::array<double, 2>& right_joint_limits)
: _ros_node(std::move(node)),
  _params(params)
{
  // if (!left_door_joint_name.empty() && left_door_joint_name != "empty_joint")
  //   _doors.insert(std::make_pair(left_door_joint_name,
  //     std::make_shared<DoorCommon::DoorElement>(
  //       left_joint_limits[0], left_joint_limits[1])));
  
  // if (!right_door_joint_name.empty() && right_door_joint_name != "empty_joint")
  //   _doors.insert(std::make_pair(right_door_joint_name,
  //     std::make_shared<DoorCommon::DoorElement>(
  //       right_joint_limits[0], right_joint_limits[1])));

  if (!left_door_joint_name.empty() && left_door_joint_name != "empty_joint")
  {
    DoorCommon::DoorElement door_element(left_joint_limits[0], left_joint_limits[1]);
    _doors.insert({left_door_joint_name, door_element});
  }
  
  if (!right_door_joint_name.empty() && right_door_joint_name != "empty_joint")
  {
    DoorCommon::DoorElement door_element(right_joint_limits[0], right_joint_limits[1]);
    _doors.insert({left_door_joint_name, door_element});
  }

  _state.door_name = door_name;
  _request.requested_mode.value = DoorMode::MODE_CLOSED;

  _door_state_pub = _ros_node->create_publisher<DoorState>(
    "/door_states", rclcpp::SystemDefaultsQoS());

  _door_request_sub = _ros_node->create_subscription<DoorRequest>(
    "/door_requests", rclcpp::SystemDefaultsQoS(),
    [&](DoorRequest::UniquePtr msg)
    {
      if (msg->door_name == _state.door_name)
        _request = *msg;
    });

  _initialized = true;
}

bool DoorCommon::all_doors_open()
{
  // for (const auto& door : _doors)
  //   if (std::abs(door.second.open_position
  //     - door.second.current_position) > _params.dx_min)
  //     return false;

  for (const auto& door : _doors)
    if (std::abs(door.second.open_position
      - door.second.current_position) > _params.dx_min)
      return false;

  return true;
}

bool DoorCommon::all_doors_closed()
{
  for (const auto& door : _doors)
  {
    if (std::abs(door.second.closed_position
      - door.second.current_position) > _params.dx_min)
        return false;
  }

  return true;
}

double DoorCommon::calculate_target_velocity(
  const double target,
  const double current_position,
  const double current_velocity,
  const double dt)
{
  double dx = target - current_position;
  if (std::abs(dx) < _params.dx_min/2.0)
    dx = 0.0;

  double door_v = compute_desired_rate_of_change(
    dx, current_velocity, _params, dt);
  
  return door_v;
}

std::vector<DoorCommon::DoorUpdateResult> DoorCommon::update(
  const double time,
  const std::vector<DoorCommon::DoorUpdateRequest>& requests)
{
  double dt = time - _last_update_time;
  _last_update_time = time;

  // Update simulation position and velocity of each joint and
  // calcuate target velocity for the same
  std::vector<DoorCommon::DoorUpdateResult> results;
  for (const auto& request : requests)
  {
    const auto it = _doors.find(request.joint_name);
    if (it != _doors.end())
    {
      it->second.current_position = request.position;
      it->second.current_velocity = request.velocity;
      DoorCommon::DoorUpdateResult result;
      result.joint_name = request.joint_name;
      result.fmax = _params.f_max;
      if (requested_mode().value == DoorMode::MODE_OPEN)
      {
        result.velocity = calculate_target_velocity(
            it->second.open_position,
            request.position,
            request.velocity,
            dt);
      }
      else
      {
        result.velocity = calculate_target_velocity(
          it->second.closed_position,
          request.position,
          request.velocity,
          dt);
      }
      results.push_back(result);
    }
    else
    {
      RCLCPP_ERROR(logger(),
        "Received update request for uninitialized joint [%s]",
        request.joint_name.c_str());
    }
  }
  
  // Publishing door states
  if (time - _last_pub_time >= 1.0)
  {
    _last_pub_time = time;
    const int32_t t_sec = static_cast<int32_t>(time);
    const uint32_t t_nsec =
      static_cast<uint32_t>((time-static_cast<double>(t_sec)) *1e9);
    const rclcpp::Time now{t_sec, t_nsec, RCL_ROS_TIME};

    if (all_doors_open())
    {
      publish_state(DoorMode::MODE_OPEN, now);
    }
    else if (all_doors_closed())
    {
      publish_state(DoorMode::MODE_CLOSED, now);
    }
    else
    {
      publish_state(DoorMode::MODE_MOVING, now);
    }
  }

  return results;
}


} // namespace building_sim_common
