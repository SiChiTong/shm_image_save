/**
 * nodelet to save image message
 * Copyright(C) 2020 Isao Hara
 * License: Apache License v2.0
 */
#include <shm_image_save/plugin_nodelet_shm_saver.h>

namespace shm_image_save
{
/*
  Map a shared memory for camera images
*/
cam_shm_data *
map_cam_shm(int *shmid, int id)
{
  void *res;
  res=map_shared_mem(shmid, id, SHM_IMAGE_LEN,0);
  if(res == NULL){
    res=map_shared_mem(shmid, id, SHM_IMAGE_LEN,1);
    if (res < 0){
      fprintf(stderr, "Fail to map cam_shm_data\n");
      return NULL;
    }
    memset(res, 0, SHM_IMAGE_LEN);
  }
  return (cam_shm_data *)res;
}

/*
 * Constructor
 */
void
plugin_nodelet_shm_saver::onInit()
{
  nh_ = getNodeHandle();
  m_cam_ = map_cam_shm(&m_shmid_, SHM_IMAGE_ID);
  if (m_cam_ != NULL){
    cam_info_sub_ = nh_.subscribe(CAMERA_INFO, 1,
                       &plugin_nodelet_shm_saver::callbackCameraInfo,this);
    cam_image_sub_ = nh_.subscribe(CAMERA_IMAGE, 1,
                       &plugin_nodelet_shm_saver::callbackColorImage,this);
    rs_cloud_sub_ = nh_.subscribe(PCL_MESSAGE, 1,
                       &plugin_nodelet_shm_saver::callbackRsCloud,this);
    ROS_INFO("Initialize shm_saver nodelet");
  }
  return;
}

/**
 * callback to copy camera_info
 */
void 
plugin_nodelet_shm_saver::callbackCameraInfo(
      const sensor_msgs::CameraInfoConstPtr &msg)
{
  uint32_t serial_size = ros::serialization::serializationLength(*msg);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *msg);

  m_cam_->cam_info_size = serial_size;
  int pos = (m_cam_->cam_info_count+1) % MAX_MSGS;
  int offset = m_cam_->cam_info_size * pos;

  memcpy(m_cam_->data + offset, (void *)buffer.get(), serial_size);

  m_cam_->cam_info_offset = offset;
  m_cam_->cam_info_count = pos;
  return;
}

/**
 *
 * callback to copy image data
 */
void 
plugin_nodelet_shm_saver::callbackColorImage(
   const sensor_msgs::ImageConstPtr& msg)

{
  uint32_t serial_size = ros::serialization::serializationLength(*msg);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *msg);

  m_cam_->cam_image_size = serial_size;
  int pos = (m_cam_->cam_image_count+1) % MAX_MSGS;
  int offset = m_cam_->cam_info_size * MAX_MSGS 
               + m_cam_->cam_image_size * pos;

  memcpy(m_cam_->data + offset, (void *)buffer.get(), serial_size);

  m_cam_->cam_image_offset = offset;
  m_cam_->cam_image_count = pos;
  return;
}

/**
 *
 * callback to copy PointCloud2 data
 */
void 
plugin_nodelet_shm_saver::callbackRsCloud(
   const sensor_msgs::PointCloud2ConstPtr &msg)
{
  uint32_t serial_size = ros::serialization::serializationLength(*msg);
  boost::shared_array<uint8_t> buffer(new uint8_t[serial_size]);
  ros::serialization::OStream stream(buffer.get(), serial_size);
  ros::serialization::serialize(stream, *msg);

  m_cam_->pcl_size = serial_size;

  int pos = (m_cam_->pcl_count+1) % MAX_MSGS;
  int offset = m_cam_->cam_info_size * MAX_MSGS 
               + m_cam_->cam_image_size * MAX_MSGS
               + m_cam_->pcl_size * pos;

  memcpy(m_cam_->data + offset, (void *)buffer.get(), serial_size);

  m_cam_->pcl_offset = offset ;
  m_cam_->pcl_count = pos;
  return;
}

}

PLUGINLIB_EXPORT_CLASS(shm_image_save::plugin_nodelet_shm_saver, nodelet::Nodelet);

