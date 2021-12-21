#include <iostream>
#include <stdio.h>
#include <math.h>
#include <bits/stdc++.h>
#include <Eigen/Dense>
#include <Eigen/Core>
#include <Eigen/LU>
#include <string>
#include <ros/ros.h>
#include <geometry_msgs/Pose.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl_ros/point_cloud.h>
#include <pcl_ros/transforms.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl/features/normal_3d.h>
#include <pcl/point_types.h>
#include <geometry_msgs/PointStamped.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/filters/crop_box.h>
#include <pcl/filters/conditional_removal.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/keypoints/harris_3d.h>
#include <pcl/common/transforms.h>
#include <time.h>
#include <pcl/features/normal_3d_omp.h>
#include <fstream>
#include <limits>
#include <pcl/filters/statistical_outlier_removal.h>

#include <pcl/filters/crop_box.h>
#include <pcl/filters/extract_indices.h>
#include <pcl_ros/point_cloud.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud_conversion.h>
#include <pcl/io/pcd_io.h>
#include <eigen_conversions/eigen_msg.h>
#include <tf_conversions/tf_eigen.h>

#include <tf2_ros/static_transform_broadcaster.h>//tf2関連
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/convert.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2/LinearMath/Quaternion.h>//tf2::transform使えるようにする
#include <tf2_eigen/tf2_eigen.h>

#include <pcl/people/person_cluster.h> //角度について
#include <algorithm>
#include <vector>

#include "openpose_ros_msgs/PointWithProb.h"

#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <std_msgs/UInt32.h>//UInt16型のトピックを受け取れるようになる

//pcl::PointXYZ-位置座標のみ
//pcl::PointXYZRGB-位置座標＋色情報
//pcl::PointXYZRGBA-位置座標＋色情報＋透明度情報
//pcl::PointNormal-位置座標＋法線情報＋曲率情報

using namespace Eigen;
typedef pcl::PointXYZRGBA PointT;

class calibration_pointing {
private:
	ros::Subscriber sub_;
	ros::Subscriber sub_Nose;
	ros::Subscriber sub_LEye;
	ros::Subscriber sub_Wrist;
	ros::Subscriber sub_Elbow;
	ros::Subscriber sub_Fingertip;
	ros::Subscriber sub_First_joint;
	ros::Subscriber sub_Second_joint;
	ros::Subscriber sub_Third_joint;
	ros::Subscriber sub_flag_line;

	ros::Publisher pub_plane;
	ros::Publisher pub_rest;
	ros::Publisher pub_table_plane;
	ros::Publisher pub_table_rest;

	ros::Publisher pub_trimming;
	ros::Publisher pub_tube;
	ros::Publisher pub_dwnsmp;
	ros::Publisher pub_cluster_0;
	ros::Publisher pub_cluster_1;
	ros::Publisher pub_cluster_2;
	ros::Publisher pub_cluster_3;
	ros::Publisher pub_tube_cluster;
	ros::Publisher pub_tube_points;

	ros::Publisher pub_tube_ave_point;
	ros::Publisher pub_plane_ave_point;
	ros::Publisher pub_cluster_ave_point;
	ros::Publisher pub_tube_centroid_point;
	ros::Publisher pub_cluster_centroid_point;

	ros::Publisher pub_tube_pose;
	ros::Publisher pub_plane_pose;
	ros::Publisher pub_Nose_fingertip_pose;

	ros::Publisher pub_Nose;
	ros::Publisher pub_LEye;
	ros::Publisher pub_Wrist;
	ros::Publisher pub_Elbow;
	ros::Publisher pub_Fingertip;
	ros::Publisher pub_First_joint;
	ros::Publisher pub_Second_joint;
	ros::Publisher pub_Third_joint;
	ros::Publisher pub_line;
	ros::Publisher pub_trimming_line;
	ros::Publisher pub_cluster_best;
	ros::Publisher pub_line_fingertip;
	ros::Publisher pub_line_first;
	ros::Publisher pub_line_second;
	ros::Publisher pub_line_third;
	ros::Publisher pub_line_destination;
	ros::Publisher pub_calibration_destination;


	float threshold_plane = 0.035;//閾値(値が大きいほど除去が厳しくなる)
	float threshold_tube = 0.0804;
	int number_neighbors = 20;
	float radius_min_limits = 0.015;
	float radius_max_limits = 0.04;

	float x_Wrist = 0.0;
	float x_Nose = 0.0;
	float x_LEye = 0.0;
	float x_Elbow = 0.0;
	float x_Fingertip = 0.0;
	float x_First_joint = 0.0;
	float x_Second_joint = 0.0;
	float x_Third_joint = 0.0;

	float y_Wrist = 0.0;
	float y_Nose = 0.0;
	float y_LEye = 0.0;
	float y_Elbow = 0.0;
	float y_Fingertip = 0.0;
	float y_First_joint = 0.0;
	float y_Second_joint = 0.0;
	float y_Third_joint = 0.0;

	float z_Wrist = 0.0;
	float z_Nose = 0.0;
	float z_LEye = 0.0;
	float z_Elbow = 0.0;
	float z_Fingertip = 0.0;
	float z_First_joint = 0.0;
	float z_Second_joint = 0.0;
	float z_Third_joint = 0.0;

	//仰角の補正値
	double deg_fingertip,deg_first,deg_second,deg_third;//角度
  double rad_fingertip,rad_first,rad_second,rad_third;//ラジアン
	//基準となる関節の仰角較正値
	double rad_best=0.0;
	double dis_best,x_best,y_best,z_best;

	int flag_line=1;
	int min_number=0;

  //保管用の指差し延長線
	pcl::PointCloud<PointT>cloud_line;
	pcl::PointCloud<PointT>cloud_trimming_line;
  pcl::PointCloud<PointT> cloud_line_fingertip;
	pcl::PointCloud<PointT> cloud_calibration_fingertip;//較正後
	pcl::PointCloud<PointT> cloud_line_first;
	pcl::PointCloud<PointT> cloud_calibration_first;
	pcl::PointCloud<PointT> cloud_line_second;
	pcl::PointCloud<PointT> cloud_calibration_second;
	pcl::PointCloud<PointT> cloud_line_third;
	pcl::PointCloud<PointT> cloud_calibration_third;
	pcl::PointCloud<PointT> cloud_line_destination;//較正前の直線
	pcl::PointCloud<PointT> cloud_calibration_destination;//較正後の直線

public:
	calibration_pointing() {
		ros::NodeHandle nh("~");
		nh.param<float>("threshold_plane", threshold_plane, threshold_plane);
		nh.param<float>("threshold_tube", threshold_tube, threshold_tube);
		nh.param<float>("radius_min_limits", radius_min_limits, radius_min_limits);
		nh.param<float>("radius_max_limits", radius_max_limits, radius_max_limits);

		// sub_ = nh.subscribe("/camera/depth/color/points", 1, &calibration_pointing::cloud_callback, this);//realsense用
		sub_ = nh.subscribe("/hsrb/head_rgbd_sensor/depth_registered/rectified_points", 1, &calibration_pointing::cloud_callback, this);
		sub_Nose = nh.subscribe("/detect_poses_node/Nose_pose", 1, &calibration_pointing::Nose_callback, this);
		sub_LEye = nh.subscribe("/detect_poses_node/LEye_pose", 1, &calibration_pointing::LEye_callback, this);
		sub_Wrist = nh.subscribe("/detect_poses_node/Wrist_pose", 1, &calibration_pointing::Wrist_callback, this);
		sub_Elbow = nh.subscribe("/detect_poses_node/Elbow_pose", 1, &calibration_pointing::Elbow_callback, this);
		sub_Fingertip = nh.subscribe("/detect_poses_node/Fingertip_pose", 1, &calibration_pointing::Fingertip_callback, this);
		sub_First_joint = nh.subscribe("/detect_poses_node/First_joint_pose", 1, &calibration_pointing::First_joint_callback, this);
		sub_Second_joint = nh.subscribe("/detect_poses_node/Second_joint_pose", 1, &calibration_pointing::Second_joint_callback, this);
		sub_Third_joint = nh.subscribe("/detect_poses_node/Third_joint_pose", 1, &calibration_pointing::Third_joint_callback, this);
		sub_flag_line = nh.subscribe("/flag_line", 1, &calibration_pointing::Flag_line_callback, this);

		pub_plane = nh.advertise<sensor_msgs::PointCloud2>("cloud_plane", 1);
		pub_rest = nh.advertise<sensor_msgs::PointCloud2>("cloud_rest", 1);
		pub_table_plane = nh.advertise<sensor_msgs::PointCloud2>("cloud_table_plane", 1);
		pub_table_rest = nh.advertise<sensor_msgs::PointCloud2>("cloud_table_rest", 1);
		pub_tube = nh.advertise<sensor_msgs::PointCloud2>("tube", 1);
		pub_dwnsmp =  nh.advertise<sensor_msgs::PointCloud2>("dwnsmp", 1);
		pub_cluster_0 =  nh.advertise<sensor_msgs::PointCloud2>("cluster_0", 1);
		pub_cluster_1 =  nh.advertise<sensor_msgs::PointCloud2>("cluster_1", 1);
		pub_cluster_2 =  nh.advertise<sensor_msgs::PointCloud2>("cluster_2", 1);
		pub_cluster_3 =  nh.advertise<sensor_msgs::PointCloud2>("cluster_3", 1);
		pub_tube_cluster =  nh.advertise<sensor_msgs::PointCloud2>("tube_cluster", 1);
		pub_tube_points =  nh.advertise<sensor_msgs::PointCloud2>("tube_points", 1);
		pub_Nose = nh.advertise<sensor_msgs::PointCloud2>("Nose",1);
		pub_LEye = nh.advertise<sensor_msgs::PointCloud2>("LEye",1);
		pub_Wrist = nh.advertise<sensor_msgs::PointCloud2>("Wrist",1);
		pub_Elbow = nh.advertise<sensor_msgs::PointCloud2>("Elbow",1);
		pub_Fingertip = nh.advertise<sensor_msgs::PointCloud2>("Fingertip",1);
		pub_First_joint = nh.advertise<sensor_msgs::PointCloud2>("First_joint",1);
		pub_Second_joint = nh.advertise<sensor_msgs::PointCloud2>("Second_joint",1);
		pub_Third_joint = nh.advertise<sensor_msgs::PointCloud2>("Third_joint",1);
		pub_line = nh.advertise<sensor_msgs::PointCloud2>("line",1);
		pub_trimming_line = nh.advertise<sensor_msgs::PointCloud2>("trimming_line",1);
		pub_cluster_best = nh.advertise<sensor_msgs::PointCloud2>("cluster_best",1);
		pub_line_fingertip = nh.advertise<sensor_msgs::PointCloud2>("line_fingertip",1);
		pub_line_first = nh.advertise<sensor_msgs::PointCloud2>("line_first",1);
		pub_line_second = nh.advertise<sensor_msgs::PointCloud2>("line_second",1);
		pub_line_third = nh.advertise<sensor_msgs::PointCloud2>("line_third",1);
		pub_line_destination = nh.advertise<sensor_msgs::PointCloud2>("line_destination",1);
		pub_calibration_destination = nh.advertise<sensor_msgs::PointCloud2>("calibration_destination",1);

		pub_tube_ave_point = nh.advertise<geometry_msgs::PointStamped>("tube_ave_point", 1);
		pub_plane_ave_point = nh.advertise<geometry_msgs::PointStamped>("plane_ave_point", 1);
		pub_tube_centroid_point = nh.advertise<geometry_msgs::PointStamped>("tube_centroid_point", 1);
		pub_cluster_ave_point = nh.advertise<geometry_msgs::PointStamped>("cluster_ave_point", 1);
		pub_cluster_centroid_point = nh.advertise<geometry_msgs::PointStamped>("cluster_centroid_point", 1);

		pub_tube_pose = nh.advertise<geometry_msgs::Pose>("tube_pose", 1);
		pub_plane_pose = nh.advertise<geometry_msgs::Pose>("planar_pose", 1);
		pub_Nose_fingertip_pose = nh.advertise<geometry_msgs::Pose>("Nose_fingertip_pose", 1);

	}
	tf2_ros::TransformBroadcaster br_plane;
	tf2::Transform transform_mult;//回転や並進などの変換をサポートするクラス

	tf2_ros::TransformBroadcaster br_tube;
	tf2::Transform transform_pub;

	tf2::Transform transform_mult_mult;
	tf2_ros::Buffer tfBuffer;

	//========出力関数=========================================================================
	void Output_pub(pcl::PointCloud<PointT>::Ptr cloud_,sensor_msgs::PointCloud2 msgs_,ros::Publisher pub_){
		if(cloud_->size() <= 0){
		}
		pcl::toROSMsg(*cloud_, msgs_);
		 // msgs_.header.frame_id = "camera_depth_optical_frame";
		 msgs_.header.frame_id = "head_rgbd_sensor_rgb_frame";
		pub_.publish(msgs_);
	}

	void Output_pub_MAP(pcl::PointCloud<PointT>::Ptr cloud_,sensor_msgs::PointCloud2 msgs_,ros::Publisher pub_){
		if(cloud_->size() <= 0){
		}
		pcl::toROSMsg(*cloud_, msgs_);
		msgs_.header.frame_id = "map";
		pub_.publish(msgs_);
	}

	//========出力関数(head_rgbd_sensor_rgb_frame基準の生の点群)===========================================================
	void Output_pub_raw(pcl::PointCloud<PointT> cloud_,sensor_msgs::PointCloud2 msgs_,ros::Publisher pub_){
		if(cloud_.size() <= 0){
		}
		pcl::toROSMsg(cloud_, msgs_);
		 msgs_.header.frame_id = "head_rgbd_sensor_rgb_frame";
		pub_.publish(msgs_);
	}

	//========出力関数(map基準の生の点群)=========================================================================
	void Output_pub_map(pcl::PointCloud<PointT> cloud_,sensor_msgs::PointCloud2 msgs_,ros::Publisher pub_){
		if(cloud_.size() <= 0){
		}
		pcl::toROSMsg(cloud_, msgs_);
		 msgs_.header.frame_id = "map";
		pub_.publish(msgs_);
	}

//ダウンサンプリング===============================================================================================
	pcl::PointCloud<PointT>::Ptr Down_Sampling(pcl::PointCloud<PointT>::Ptr cloud_input)
	{
		pcl::PointCloud<PointT>::Ptr cloud_vg (new pcl::PointCloud<PointT>);//格納用

		pcl::VoxelGrid<PointT> vg;//空間をボクセルで区切り、点群の重心で近似し、ダウンサンプリングを行う
		vg.setInputCloud (cloud_input);//入力データセットへのポインタを提供する
		vg.setLeafSize (0.005f, 0.005f, 0.005f);//サイズが0.5*0.5*0.5のフィルターを作成
		vg.filter (*cloud_vg);//入力データを渡し、計算、出力
		return cloud_vg;
	}


//サーフェス法線の推定==================================================
	pcl::PointCloud<pcl::Normal>::Ptr Estimated_Normal_OMP(pcl::PointCloud<PointT>::Ptr cloud_input)
	{
		pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal>);//法線情報を入れる変数
		pcl::search::KdTree<PointT>::Ptr tree (new pcl::search::KdTree<PointT> ());//KdTree構造をつくる
		pcl::NormalEstimationOMP<PointT, pcl::Normal> ne;//サーフェス曲線と曲率を推定する（Normal型点群）

		//法線の推定================
		ne.setSearchMethod (tree);//推定方法
		ne.setInputCloud (cloud_input);//推定に用いる点群
		ne.useSensorOriginAsViewPoint();//センサの原点を使用するか、ユーザが指定した視点を使用するかの設定
		ne.setNumberOfThreads(0);
		ne.setKSearch (50);//特徴推定に使用するｋ最近傍の数を所得する
		ne.compute (*cloud_normals);//推定するための計算（出力先はclod_normals）
		return cloud_normals;
	}


	pcl::PointCloud<PointT>::Ptr Detected_tube(pcl::PointCloud<PointT>::Ptr cloud_input, pcl::PointCloud<pcl::Normal>::Ptr normal_input)
	{
		//円柱を検出する=============
		pcl::SACSegmentationFromNormals<PointT, pcl::Normal> seg;//円柱を検出するクラス
		pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
		pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
		seg.setInputCloud(cloud_input);
		seg.setInputNormals (normal_input);
		seg.setModelType(pcl::SACMODEL_CYLINDER);//円柱モデルの指定
		seg.setMethodType(pcl::SAC_RANSAC);
		seg.setNormalDistanceWeight (0.01);//点法線と平面法線の間の角距離に与える相対的な重み
		seg.setMaxIterations (10000);//試行回数
		seg.setDistanceThreshold(threshold_tube);//閾値設定
		seg.setRadiusLimits (radius_min_limits, radius_max_limits);//モデルの最小・最大許容半径制限を設定する
		seg.setOptimizeCoefficients(true);//係数の情報必要かどうかの設定
		seg.segment(*inliers, *coefficients);
		//std::cout << *coefficients << std::endl;
		// 円柱以外を除去する。＝＝＝＝＝＝＝＝＝＝
		pcl::PointCloud<PointT>::Ptr cloud_tube(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_outside_tube(new pcl::PointCloud<PointT>);
		pcl::ExtractIndices<PointT> extract;
		extract.setInputCloud(cloud_input);
		extract.setIndices(inliers);
		extract.setNegative(false);
		extract.filter(*cloud_tube);
		//std::cout << *cloud_tube << std::endl;
		return cloud_tube;
	}

	Eigen::Vector4f ave_point_of_PointCloud(pcl::PointCloud<PointT>::Ptr cloud_input, ros::Publisher pub_container, geometry_msgs::PointStamped msgs_container)
	{       //Eigen::Vector4f-int型の四次元ベクトル
					//geometry_msgs-姿勢
		      //平均を計算する
		float  x_sum  = 0.0;
		float  y_sum  = 0.0;
		float  z_sum  = 0.0;
		for(size_t i = 0; i< cloud_input->size(); ++i)
		{
			 x_sum += cloud_input->points[i].x;
			 y_sum += cloud_input->points[i].y;
			 z_sum += cloud_input->points[i].z;
		}
		Eigen::Vector4f ave_point;//格納
		ave_point.x() =  x_sum/static_cast<int>(cloud_input->size());
		ave_point.y() =  y_sum/static_cast<int>(cloud_input->size());
		ave_point.z() =  z_sum/static_cast<int>(cloud_input->size());

		msgs_container.header.stamp = ros::Time::now();//現在時刻
		msgs_container.point.x = ave_point.x();
		msgs_container.point.y = ave_point.y();
		msgs_container.point.z = ave_point.z();

		return ave_point;
	}

	Eigen::Vector4f centroid_point_of_PointCloud(pcl::PointCloud<PointT>::Ptr cloud_input, ros::Publisher pub_container, geometry_msgs::PointStamped msgs_container)
	{
		//中心座標の出力===================================================
		Eigen::Vector4f centroid_point;
		pcl::compute3DCentroid(*cloud_input, centroid_point);//重心を計算
                                                         //3Dベクトルとして返す
		msgs_container.header.stamp = ros::Time::now();
		msgs_container.point.x = centroid_point.x();
		msgs_container.point.y = centroid_point.y();
		msgs_container.point.z = centroid_point.z();

		pub_container.publish(msgs_container);

		return centroid_point;
	}

	void publish_pose(tf2::Transform input_transform, ros::Publisher pub_container, geometry_msgs::Pose msgs_container)
	{                                                                                         //Pose型：点群の位置、姿勢を表す
		msgs_container.position.x = input_transform.getOrigin().x();//原点ベクトルの平行移動を返す
		msgs_container.position.y = input_transform.getOrigin().y();
		msgs_container.position.z = input_transform.getOrigin().z();

		msgs_container.orientation.x = input_transform.getRotation().x();//回転を表すクォータニオンを返す
		msgs_container.orientation.y = input_transform.getRotation().y();
		msgs_container.orientation.z = input_transform.getRotation().z();
		msgs_container.orientation.w = input_transform.getRotation().w();

		pub_container.publish(msgs_container);
		return;
	}

	//点群のフィルタリング
	pcl::PointCloud<PointT>::Ptr Pass_Through(pcl::PointCloud<PointT>::Ptr cloud_input,float x_min,float x_max,float y_min,float y_max,float z_min,float z_max)
	{
		pcl::PassThrough<PointT>pass;
		pcl::PointCloud<PointT>::Ptr cloud_passthrough_x(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_passthrough_y(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_passthrough_z(new pcl::PointCloud<PointT>);

		pass.setInputCloud(cloud_input);
		pass.setFilterFieldName("x");
		pass.setFilterLimits(x_min,x_max);
		pass.filter(*cloud_passthrough_x);

		pass.setInputCloud(cloud_passthrough_x);
		pass.setFilterFieldName("y");
		pass.setFilterLimits(y_min,y_max);
		pass.filter(*cloud_passthrough_y);

		pass.setInputCloud(cloud_passthrough_y);
		pass.setFilterFieldName("z");
		pass.setFilterLimits(z_min,z_max);
		pass.filter(*cloud_passthrough_z);

		return cloud_passthrough_z;
	}

	//transform========================================================================================
	pcl::PointCloud<PointT>::Ptr Transform_points(pcl::PointCloud<PointT>::Ptr cloud_input, pcl::PointCloud<PointT>::Ptr cloud_output, Eigen::Vector4f plane_ave_point, tf2::Vector3 plane_axis,float plane_angle)
	{
		//座標変換(平行移動→回転移動)================================================================
		pcl::PointCloud<PointT>::Ptr cloud_transform(new pcl::PointCloud<PointT>);
		Eigen::Affine3f affine_translation=Eigen::Affine3f::Identity();
		affine_translation.translation()<<-plane_ave_point.x(),-plane_ave_point.y(),-plane_ave_point.z();
		pcl::transformPointCloud(*cloud_input,*cloud_transform,affine_translation);
		Eigen::Affine3f affine_rotation=Eigen::Affine3f::Identity();
		affine_rotation.rotate(Eigen::AngleAxisf(-plane_angle,Eigen::Vector3f(plane_axis.x(), plane_axis.y(), plane_axis.z())));
		pcl::transformPointCloud(*cloud_transform,*cloud_output,affine_rotation);
		return cloud_output;
	}

	//transform_return=====================================================================================
	pcl::PointCloud<PointT>::Ptr Transform_return_points(pcl::PointCloud<PointT>::Ptr cloud_input, pcl::PointCloud<PointT>::Ptr cloud_output, Eigen::Vector4f plane_ave_point, tf2::Vector3 plane_axis,float plane_angle)
	{
		//元の座標に戻す(回転移動→平行移動)
	 pcl::PointCloud<PointT>::Ptr cloud_transform_return(new pcl::PointCloud<PointT>);
	 Eigen::Affine3f affine_translation_return=Eigen::Affine3f::Identity();
	 affine_translation_return.rotate(Eigen::AngleAxisf(plane_angle,Eigen::Vector3f(plane_axis.x(), plane_axis.y(), plane_axis.z())));
	 pcl::transformPointCloud(*cloud_input,*cloud_transform_return,affine_translation_return);
	 Eigen::Affine3f affine_rotation_return=Eigen::Affine3f::Identity();
	 affine_rotation_return.translation()<<plane_ave_point.x(),plane_ave_point.y(),plane_ave_point.z();
	 pcl::transformPointCloud(*cloud_transform_return,*cloud_output,affine_rotation_return);
	 return cloud_output;
 }

 //仰角による基準点の較正関数======================================================================================================================
 pcl::PointCloud<PointT> Calibration_angle( geometry_msgs::TransformStamped transform_line_after,double x_Nose ,double y_Nose ,double z_Nose,double x_input ,double y_input ,double z_input,double rad_input)
 {
	 //二点のmap基準の変換====
	  pcl::PointCloud<PointT> cloud_line;
	  cloud_line.width = 1000;
    cloud_line.height = 1;
    cloud_line.points.resize(cloud_line.width * cloud_line.height);

 		cloud_line.points[0].x=x_Nose;
 		cloud_line.points[0].y=y_Nose;
 		cloud_line.points[0].z=z_Nose;

 		cloud_line.points[1].x=x_input;
 		cloud_line.points[1].y=y_input;
 		cloud_line.points[1].z=z_input;

	 //map基準に座標変換
	 pcl::PointCloud<PointT> cloud_output;
	 sensor_msgs::PointCloud2 conv_cloud_msg;
	 sensor_msgs::PointCloud2 cloud_msg_line;
	 pcl::toROSMsg(cloud_line, cloud_msg_line);
	 cloud_msg_line.header.frame_id = "head_rgbd_sensor_rgb_frame";
	 pcl::PointCloud<PointT>::Ptr cloud_input_line(new pcl::PointCloud<PointT>);
	 std::vector<int> index_all_line;
	 Eigen::Matrix4f mat = tf2::transformToEigen(transform_line_after.transform).matrix().cast<float>();
	 pcl_ros::transformPointCloud(mat,cloud_msg_line, conv_cloud_msg);
	 pcl::fromROSMsg(conv_cloud_msg, *cloud_input_line);                                         //msg→pointcloud
	 pcl::removeNaNFromPointCloud(*cloud_input_line,cloud_output,index_all_line);

	 double rad_z,vec_z,deg_z;
	 double a,b,c;//一時保管用
	 if(cloud_output.size()>0){
	 	//基準点をmapの位置に平行移動
	 	cloud_output.points[1].x-=cloud_output.points[0].x;
	 	cloud_output.points[1].y-=cloud_output.points[0].y;
	 	cloud_output.points[1].z-=cloud_output.points[0].z;
	 	//z軸(0,0,1)が基準になるように角度算出
	 	vec_z=(cloud_output.points[1].y)/(sqrt(pow(cloud_output.points[1].y,2.0)+pow(cloud_output.points[1].x,2.0)));
	 	rad_z=acos(vec_z);
	 	deg_z=rad_z*180.0/M_PI;

	 	//回転移動_Z軸方向
	 	a=cloud_output.points[1].x;
	 	b=cloud_output.points[1].y;
	 	cloud_output.points[1].x=cos(rad_z)*a-sin(rad_z)*b;
	 	cloud_output.points[1].y=sin(rad_z)*a+cos(rad_z)*b;

	 	//仰角の補正(y-z)
	 	a=cloud_output.points[1].y;
	 	b=cloud_output.points[1].z;
	 	cloud_output.points[1].y=cos(rad_input)*a-sin(rad_input)*b;
	 	cloud_output.points[1].z=sin(rad_input)*a+cos(rad_input)*b;

	 	//回転移動_z軸逆方向でもとに戻す
	 	a=cloud_output.points[1].x;
	 	b=cloud_output.points[1].y;
	 	cloud_output.points[1].x=cos(-rad_z)*a-sin(-rad_z)*b;
	 	cloud_output.points[1].y=sin(-rad_z)*a+cos(-rad_z)*b;
	 	//基準点を元の位置に平行移動
	 	cloud_output.points[1].x+=cloud_output.points[0].x;
	 	cloud_output.points[1].y+=cloud_output.points[0].y;
	 	cloud_output.points[1].z+=cloud_output.points[0].z;

		//直線にする
		a=cloud_output.points[1].x;
	 	b=cloud_output.points[1].y;
		c=cloud_output.points[1].z;

		for(int t=0;t<cloud_output.points.size();t++){
			cloud_output.points[t].x=cloud_output.points[0].x+(a-cloud_output.points[0].x)*t/100;
			cloud_output.points[t].y=cloud_output.points[0].y+(b-cloud_output.points[0].y)*t/100;
			cloud_output.points[t].z=cloud_output.points[0].z+(c-cloud_output.points[0].z)*t/100;
		}

	 }
	 return cloud_output;
	}

	Eigen::Vector4f Estimate_point(pcl::PointCloud<PointT> cloud_input,double input_z){//全てmap基準
		Eigen::Vector4f estimation_point;
		for(int i=0;i<cloud_input.size();i++){
			if(cloud_input.points[i].z >=input_z-0.005&&cloud_input.points[i].z <=input_z+0.005){
				estimation_point.x()=cloud_input.points[i].x;
				estimation_point.y()=cloud_input.points[i].y;
				estimation_point.z()=cloud_input.points[i].z;
				break;
			}
		}
		return estimation_point;
	}

	void Nose_callback(const geometry_msgs::Pose input_pose)
	{
		x_Nose=input_pose.position.x;
		y_Nose=input_pose.position.y;
		z_Nose=input_pose.position.z;
	  // std::cout << "x_Nose："<< x_Nose  <<" "<<"y_Nose：" <<y_Nose << " "<<"z_Nose：" <<z_Nose <<'\n';
	}

	void LEye_callback(const geometry_msgs::Pose input_pose)
	{
		x_LEye=input_pose.position.x;
		y_LEye=input_pose.position.y;
		z_LEye=input_pose.position.z;
	  // std::cout << "x_LEye："<< x_LEye  <<" "<<"y_LEye：" <<y_LEye << " "<<"z_LEye：" <<z_LEye <<'\n';
	}

	void Wrist_callback(const geometry_msgs::Pose input_pose)
	{
		x_Wrist=input_pose.position.x;
		y_Wrist=input_pose.position.y;
		z_Wrist=input_pose.position.z;
		 // std::cout << "x_Wrist："<< x_Wrist  <<" "<<"y_Wrist：" <<y_Wrist << " "<<"z_Wrist：" <<z_Wrist <<'\n';
	}

	void Elbow_callback(const geometry_msgs::Pose input_pose)
	{
		x_Elbow=input_pose.position.x;
		y_Elbow=input_pose.position.y;
		z_Elbow=input_pose.position.z;
	}

	void Fingertip_callback(const geometry_msgs::Pose input_pose)
	{
		x_Fingertip=input_pose.position.x;
		y_Fingertip=input_pose.position.y;
		z_Fingertip=input_pose.position.z;
		 // std::cout << "x_Fingertip："<< x_Fingertip  <<" "<<"y_Fingertip：" <<y_Fingertip << " "<<"z_Fingertip：" <<z_Fingertip <<'\n';
	}

	void First_joint_callback(const geometry_msgs::Pose input_pose)
	{
		x_First_joint=input_pose.position.x;
		y_First_joint=input_pose.position.y;
		z_First_joint=input_pose.position.z;
		 // std::cout << "x_First_joint："<< x_First_joint  <<" "<<"y_First_joint：" <<y_First_joint << " "<<"z_First_joint：" <<z_First_joint <<'\n';
	}

	void Second_joint_callback(const geometry_msgs::Pose input_pose)
	{
		x_Second_joint=input_pose.position.x;
		y_Second_joint=input_pose.position.y;
		z_Second_joint=input_pose.position.z;
		 // std::cout << "x_Second_joint："<< x_Second_joint  <<" "<<"y_Second_joint：" <<y_Second_joint << " "<<"z_Second_joint：" <<z_Second_joint <<'\n';
	}

	void Third_joint_callback(const geometry_msgs::Pose input_pose)
	{
		x_Third_joint=input_pose.position.x;
		y_Third_joint=input_pose.position.y;
		z_Third_joint=input_pose.position.z;
		 // std::cout << "x_Third_joint："<< x_Third_joint  <<" "<<"y_Third_joint：" <<y_Third_joint << " "<<"z_Third_joint：" <<z_Third_joint <<'\n';
	}

	void Flag_line_callback(const std_msgs::UInt32 input_data)
	{
		if(input_data.data==0){
			flag_line=0;
	  	std::cout <<"/////////////////////////////////////sub_flag_line=0"<<'\n';
		}else if(input_data.data==1){
			flag_line=1;
	  	std::cout <<"/////////////////////////////////////sub_flag_line=1"<<'\n';
		}else if(input_data.data==2){
			flag_line=2;
	  	std::cout <<"/////////////////////////////////////sub_flag_line=2"<<'\n';
		}else if(input_data.data==3){
			flag_line=3;
	  	std::cout <<"/////////////////////////////////////sub_flag_line=3"<<'\n';
		}else if(input_data.data==4){
			flag_line=4;
	  	std::cout <<"/////////////////////////////////////sub_flag_line=4"<<'\n';
		}
	}


//コールバック関数===================================================================================================================================
	void cloud_callback(const sensor_msgs::PointCloud2ConstPtr &cloud_msg)
	{
		std::cout << "calibration_pointing_start" << '\n';

		// 読み込む。==================
		pcl::PointCloud<PointT>::Ptr cloud_input(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_trimming(new pcl::PointCloud<PointT>);
		std::vector<int> index_all;
		pcl::fromROSMsg(*cloud_msg, *cloud_input);
		pcl::removeNaNFromPointCloud(*cloud_input,*cloud_trimming,index_all);


		//鼻と手首,指先の点群を切り取る======================================================================================================================
		pcl::PointCloud<PointT>::Ptr cloud_Nose(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_LEye(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_Wrist(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_Elbow(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_Fingertip (new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_First_joint(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_Second_joint(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_Third_joint(new pcl::PointCloud<PointT>);
		if(z_Nose!=0.0){
			cloud_Nose=Pass_Through(cloud_trimming,x_Nose-0.02,x_Nose+0.02,y_Nose-0.02,y_Nose+0.02,z_Nose-0.02,z_Nose+0.02);
		}
		if(z_LEye!=0.0){
			cloud_LEye=Pass_Through(cloud_trimming,x_LEye-0.02,x_LEye+0.02,y_LEye-0.02,y_LEye+0.02,z_LEye-0.02,z_LEye+0.02);
		}
		if(z_Wrist!=0.0){
			cloud_Wrist=Pass_Through(cloud_trimming,x_Wrist-0.02,x_Wrist+0.02,y_Wrist-0.02,y_Wrist+0.02,z_Wrist-0.02,z_Wrist+0.02);
		}
		if(z_Elbow!=0.0){
			cloud_Elbow=Pass_Through(cloud_trimming,x_Elbow-0.02,x_Elbow+0.02,y_Elbow-0.02,y_Elbow+0.02,z_Elbow-0.02,z_Elbow+0.02);
		}
		if(z_Fingertip!=0.0){
			cloud_Fingertip=Pass_Through(cloud_trimming,x_Fingertip-0.02,x_Fingertip+0.02,y_Fingertip-0.02,y_Fingertip+0.02,z_Fingertip-0.02,z_Fingertip+0.02);
		}
		if(z_First_joint!=0.0){
			cloud_First_joint=Pass_Through(cloud_trimming,x_First_joint-0.02,x_First_joint+0.02,y_First_joint-0.02,y_First_joint+0.02,z_First_joint-0.02,z_First_joint+0.02);
		}
		if(z_Second_joint!=0.0){
			cloud_Second_joint=Pass_Through(cloud_trimming,x_Second_joint-0.02,x_Second_joint+0.02,y_Second_joint-0.02,y_Second_joint+0.02,z_Second_joint-0.02,z_Second_joint+0.02);
		}
		if(z_Third_joint!=0.0){
			cloud_Third_joint=Pass_Through(cloud_trimming,x_Third_joint-0.02,x_Third_joint+0.02,y_Third_joint-0.02,y_Third_joint+0.02,z_Third_joint-0.02,z_Third_joint+0.02);
		}

		//指先の検出
		//指差し直線の算出===========================================================================
		float x,y,z;
		float t;

		//手首からどれだけ離れているか====================================
		double dis_Fingertip=DBL_MAX;
		double dis_First_joint=DBL_MAX;
		double dis_Second_joint=DBL_MAX;
		double dis_Third_joint=DBL_MAX;

		if(z_Wrist!=0.0&&z_Fingertip!=0.0){
			dis_Fingertip=sqrt(pow(x_Fingertip-x_Wrist,2.0)+pow(y_Fingertip-y_Wrist,2.0)+pow(z_Fingertip-z_Wrist,2.0));
		}
		if(z_Wrist!=0.0&&z_First_joint!=0.0){
			dis_First_joint=sqrt(pow(x_First_joint-x_Wrist,2.0)+pow(y_First_joint-y_Wrist,2.0)+pow(z_First_joint-z_Wrist,2.0));
		}
		if(z_Wrist!=0.0&&z_Second_joint!=0.0){
			dis_Second_joint=sqrt(pow(x_Second_joint-x_Wrist,2.0)+pow(y_Second_joint-y_Wrist,2.0)+pow(z_Second_joint-z_Wrist,2.0));
		}
		if(z_Wrist!=0.0&&z_Third_joint!=0.0){
			dis_Third_joint=sqrt(pow(x_Third_joint-x_Wrist,2.0)+pow(y_Third_joint-y_Wrist,2.0)+pow(z_Third_joint-z_Wrist,2.0));
		}


		//head_rgbd_sensor_rgb_frame→hand_palm_link==============================================================================
		tf2_ros::TransformListener tf_listener(tfBuffer);
		geometry_msgs::TransformStamped transform_palm;
		if(tfBuffer.canTransform("head_rgbd_sensor_rgb_frame", "hand_palm_link", ros::Time::now(), ros::Duration(10.0))){//tfにおけるwaitForTransformの代わり,変換できていれば１を返す
			try{
			 transform_palm=tfBuffer.lookupTransform("head_rgbd_sensor_rgb_frame", "hand_palm_link", ros::Time(0));
				}
			catch (tf2::TransformException& ex){
					ROS_ERROR("TF Exception: %s", ex.what());
					ros::Duration(1.0).sleep();
				}
			}
			//rgbd_camera→map==============================================================================
			geometry_msgs::TransformStamped transform_map;
			if(tfBuffer.canTransform("head_rgbd_sensor_rgb_frame", "map", ros::Time::now(), ros::Duration(10.0))){//tfにおけるwaitForTransformの代わり,変換できていれば１を返す
				try{
				 transform_map=tfBuffer.lookupTransform("head_rgbd_sensor_rgb_frame", "map", ros::Time(0));
					}
				catch (tf2::TransformException& ex){
						ROS_ERROR("TF Exception: %s", ex.what());
						ros::Duration(1.0).sleep();
					}
				}
			//map→rgbd_camera================================================================================
			geometry_msgs::TransformStamped transform_line_after;
			if(tfBuffer.canTransform("map", "head_rgbd_sensor_rgb_frame", ros::Time::now(), ros::Duration(10.0))){//tfにおけるwaitForTransformの代わり,変換できていれば１を返す
				try{
				 transform_line_after=tfBuffer.lookupTransform("map", "head_rgbd_sensor_rgb_frame", ros::Time(0));
					}
				catch (tf2::TransformException& ex){
						ROS_ERROR("TF Exception: %s", ex.what());
						ros::Duration(1.0).sleep();
					}
				}

		//hand_palm_linkの位置情報取得
		geometry_msgs::Vector3 vector_tmp=transform_palm.transform.translation;
		tf2::Vector3 vector_palm;
		tf2::fromMsg(vector_tmp,vector_palm);

		//mapの位置・姿勢情報取得
		geometry_msgs::Vector3 vector_tmp_map=transform_map.transform.translation;
		tf2::Vector3 vector_map;
		tf2::fromMsg(vector_tmp_map,vector_map);
		geometry_msgs::Quaternion q = transform_map.transform.rotation;
		tf2::Quaternion quat_map;
		tf2::fromMsg(q,quat_map);
		tf2::Transform tf2_map(quat_map,vector_map);

		//利き目の判定========================================================================================================
		if(z_Nose!=0.0&&z_LEye!=0.0){
			pcl::PointCloud<PointT> cloud_eye;
			cloud_eye.width = 4;
			cloud_eye.height = 1;
			cloud_eye.points.resize(cloud_eye.width * cloud_eye.height);

			cloud_eye.points[0].x=(float)vector_palm.getX(),cloud_eye.points[0].y=(float)vector_palm.getY(),cloud_eye.points[0].z=(float)vector_palm.getZ();
			cloud_eye.points[1].x=x_Nose,cloud_eye.points[1].y=y_Nose,cloud_eye.points[1].z=z_Nose;
			cloud_eye.points[2].x=x_LEye,cloud_eye.points[2].y=y_LEye,cloud_eye.points[2].z=z_LEye;
			cloud_eye.points[3].x=x_First_joint,cloud_eye.points[3].y=y_First_joint,cloud_eye.points[3].z=z_First_joint;

			pcl::PointCloud<PointT> cloud_output_eye;
		  sensor_msgs::PointCloud2 conv_cloud_msg_eye;
		  sensor_msgs::PointCloud2 cloud_msg_eye;
		  pcl::toROSMsg(cloud_eye, cloud_msg_eye);
		  cloud_msg_eye.header.frame_id = "head_rgbd_sensor_rgb_frame";
		  pcl::PointCloud<PointT>::Ptr cloud_input_eye(new pcl::PointCloud<PointT>);
		  std::vector<int> index_all_eye;
		  Eigen::Matrix4f mat_eye = tf2::transformToEigen(transform_line_after.transform).matrix().cast<float>();
		  pcl_ros::transformPointCloud(mat_eye,cloud_msg_eye, conv_cloud_msg_eye);
		  pcl::fromROSMsg(conv_cloud_msg_eye, *cloud_input_eye);                                         //msg→pointcloud
		  pcl::removeNaNFromPointCloud(*cloud_input_eye,cloud_output_eye,index_all_eye);

			//x-y平面上で考える
			double x_eye,y_eye,a,b,c,d,dis_Reye,dis_Leye=0.0;
			//右目と左目のx-y平面での一次関数算出
			// y=(cloud_output_eye.points[2].y-cloud_output_eye.points[1].y/cloud_output_eye.points[2].x-cloud_output_eye.points[1].x)*x-(cloud_output_eye.points[2].y-cloud_output_eye.points[1].y/cloud_output_eye.points[2].x-cloud_output_eye.points[1].x)*(cloud_output_eye.points[1].x)+cloud_output_eye.points[1].y;
			//目標地点と指先のx-y平面での一次関数算出
			// y=(cloud_output_eye.points[0].y-cloud_output_eye.points[3].y/cloud_output_eye.points[0].x-cloud_output_eye.points[3].x)*x-(cloud_output_eye.points[0].y-cloud_output_eye.points[3].y/cloud_output_eye.points[0].x-cloud_output_eye.points[3].x)*(cloud_output_eye.points[3].x)+cloud_output_eye.points[3].y;

			a=(cloud_output_eye.points[2].y-cloud_output_eye.points[1].y)/(cloud_output_eye.points[2].x-cloud_output_eye.points[1].x);
			b=cloud_output_eye.points[1].y-((cloud_output_eye.points[2].y-cloud_output_eye.points[1].y)/(cloud_output_eye.points[2].x-cloud_output_eye.points[1].x))*(cloud_output_eye.points[1].x);
			c=(cloud_output_eye.points[0].y-cloud_output_eye.points[3].y)/(cloud_output_eye.points[0].x-cloud_output_eye.points[3].x);
			d=cloud_output_eye.points[3].y-((cloud_output_eye.points[0].y-cloud_output_eye.points[3].y)/(cloud_output_eye.points[0].x-cloud_output_eye.points[3].x))*(cloud_output_eye.points[3].x);
			//右目と左目のx-y平面での一次関数目標地点と指先のx-y平面での一次関数の交点を算出
			if(a-c!=0.0&&a,b,c,d!=0.0){
				x_eye=(d-b)/(a-c);
				y_eye=(a*d-b*c)/(a-c);
				std::cout << "x,y=" <<x_eye<< " , "<< y_eye <<'\n';
				dis_Reye=sqrt(pow(x_eye-cloud_output_eye.points[1].x,2.0)+pow(y_eye-cloud_output_eye.points[1].y,2.0));
				dis_Leye=sqrt(pow(x_eye-cloud_output_eye.points[2].x,2.0)+pow(y_eye-cloud_output_eye.points[2].y,2.0));
				if(dis_Leye>=dis_Reye){
					x_Nose=cloud_eye.points[1].x,y_Nose=cloud_eye.points[1].y,z_Nose=cloud_eye.points[1].z;//利き目が右と判断
					std::cout << "The dominant eye is right"<<'\n';
				}else{
					x_Nose=cloud_eye.points[2].x,y_Nose=cloud_eye.points[2].y,z_Nose=cloud_eye.points[2].z;//利き目が左と判断
					std::cout << "The dominant eye is left"<<'\n';
				}
			}
		}

    //各指先の点をmap基準に変換============================================================================================================================
    pcl::PointCloud<PointT> cloud_fin;
		cloud_fin.width = 6;
		cloud_fin.height = 1;
		cloud_fin.points.resize(cloud_fin.width * cloud_fin.height);

		cloud_fin.points[0].x=x_Nose,cloud_fin.points[0].y=y_Nose,cloud_fin.points[0].z=z_Nose;
		cloud_fin.points[1].x=(float)vector_palm.getX(),cloud_fin.points[1].y=(float)vector_palm.getY(),cloud_fin.points[1].z=(float)vector_palm.getZ();
		cloud_fin.points[2].x=x_Fingertip,cloud_fin.points[2].y=y_Fingertip,cloud_fin.points[2].z=z_Fingertip;
		cloud_fin.points[3].x=x_First_joint,cloud_fin.points[3].y=y_First_joint,cloud_fin.points[3].z=z_First_joint;
		cloud_fin.points[4].x=x_Second_joint,cloud_fin.points[4].y=y_Second_joint,cloud_fin.points[4].z=z_Second_joint;
		cloud_fin.points[5].x=x_Third_joint,cloud_fin.points[5].y=y_Third_joint,cloud_fin.points[5].z=z_Third_joint;
		// cloud_fin.points[6].x=x_Nose,cloud_fin.points[6].y=y_Nose,cloud_fin.points[6].z=z_Nose;
	  pcl::PointCloud<PointT> cloud_output_fin;
	  sensor_msgs::PointCloud2 conv_cloud_msg_fin;
	  sensor_msgs::PointCloud2 cloud_msg_fin;
	  pcl::toROSMsg(cloud_fin, cloud_msg_fin);
	  cloud_msg_fin.header.frame_id = "head_rgbd_sensor_rgb_frame";
	  pcl::PointCloud<PointT>::Ptr cloud_input_fin(new pcl::PointCloud<PointT>);
	  std::vector<int> index_all_fin;
	  Eigen::Matrix4f mat_fin = tf2::transformToEigen(transform_line_after.transform).matrix().cast<float>();
	  pcl_ros::transformPointCloud(mat_fin,cloud_msg_fin, conv_cloud_msg_fin);
	  pcl::fromROSMsg(conv_cloud_msg_fin, *cloud_input_fin);                                         //msg→pointcloud
	  pcl::removeNaNFromPointCloud(*cloud_input_fin,cloud_output_fin,index_all_fin);

		//基準点をmapの位置に平行移動
		double a,b,vec_z,rad_z,deg_z,n_x,n_y,n_z;
		n_x=cloud_output_fin.points[0].x,n_y=cloud_output_fin.points[0].y,n_z=cloud_output_fin.points[0].z;

		for(int i=0;i<cloud_output_fin.size();i++){
		 	cloud_output_fin.points[i].x-=n_x,cloud_output_fin.points[i].y-=n_y,cloud_output_fin.points[i].z-=n_z;
		 	//z軸(0,0,1)が基準になるように角度算出
		 	if(i>0){
			 	vec_z=(cloud_output_fin.points[i].y)/(sqrt(pow(cloud_output_fin.points[i].y,2.0)+pow(cloud_output_fin.points[i].x,2.0)));
			 	rad_z=acos(vec_z);
			 	deg_z=rad_z*180.0/M_PI;
				//回転移動_Z軸方向
			 	a=cloud_output_fin.points[i].x;
			 	b=cloud_output_fin.points[i].y;
			 	cloud_output_fin.points[i].x=cos(rad_z)*a-sin(rad_z)*b;
			 	cloud_output_fin.points[i].y=sin(rad_z)*a+cos(rad_z)*b;
			}
		}

		double vec_fingertip=0.0,vec_first=0.0,vec_second=0.0,vec_third=0.0;
		//仰角の算出(y-z平面において)
		double line_fingertip_y=cloud_output_fin.points[2].y-cloud_output_fin.points[0].y,line_fingertip_z=cloud_output_fin.points[2].z-cloud_output_fin.points[0].z;
		double line_first_y=cloud_output_fin.points[3].y-cloud_output_fin.points[0].y,line_first_z=cloud_output_fin.points[3].z-cloud_output_fin.points[0].z;
		double line_second_y=cloud_output_fin.points[4].y-cloud_output_fin.points[0].y,line_second_z=cloud_output_fin.points[4].z-cloud_output_fin.points[0].z;
		double line_third_y=cloud_output_fin.points[5].y-cloud_output_fin.points[0].y,line_third_z=cloud_output_fin.points[5].z-cloud_output_fin.points[0].z;
		double line_destination_y=cloud_output_fin.points[1].y-cloud_output_fin.points[0].y,line_destination_z=cloud_output_fin.points[1].z-cloud_output_fin.points[0].z;//ハンドを指す時

		//1回目のキャリブレーション===================================================================================================================================================
		if(flag_line==1){
				//仰角をもとめる===============================================
				if(dis_Fingertip<=0.2&&z_Nose!=0.0){
					vec_fingertip=(line_fingertip_y*line_destination_y+line_fingertip_z*line_destination_z)/sqrt(((+pow(line_fingertip_y,2.0)+pow(line_fingertip_z,2.0))*(pow(line_destination_y,2.0)+pow(line_destination_z,2.0))));
					rad_fingertip=acos(vec_fingertip);
					deg_fingertip=rad_fingertip*180.0/M_PI;
					std::cout << "degree_fingertip=" <<deg_fingertip<<'\n';
				}
				if(dis_First_joint<=0.2&&z_Nose!=0.0){
					vec_first=(line_first_y*line_destination_y+line_first_z*line_destination_z)/sqrt((pow(line_first_y,2.0)+pow(line_first_z,2.0))*(pow(line_destination_y,2.0)+pow(line_destination_z,2.0)));
					rad_first=acos(vec_first);
					deg_first=rad_first*180.0/M_PI;
					std::cout << "degree_first=" <<deg_first<<'\n';
				}
				if(dis_Second_joint<=0.2&&z_Nose!=0.0){
					vec_second=(line_second_y*line_destination_y+line_second_z*line_destination_z)/sqrt(((pow(line_second_y,2.0)+pow(line_second_z,2.0))*(pow(line_destination_y,2.0)+pow(line_destination_z,2.0))));
					rad_second=acos(vec_second);
					deg_second=rad_second*180.0/M_PI;
					std::cout << "degree_second=" <<deg_second<<'\n';
				}
				if(dis_Third_joint<=0.2&&z_Nose!=0.0){
					vec_third=(line_third_y*line_destination_y+line_third_z*line_destination_z)/sqrt(((pow(line_third_y,2.0)+pow(line_third_z,2.0))*(pow(line_destination_y,2.0)+pow(line_destination_z,2.0))));
					rad_third=acos(vec_third);
					deg_third=rad_third*180.0/M_PI;
					std::cout << "degree_third=" <<deg_third<<'\n';
				}

				//直線の可視化================================================================
				if(dis_Fingertip<=0.2&&z_Nose!=0.0){
					 cloud_line_fingertip.width = 1000;
		       cloud_line_fingertip.height = 1;
		       cloud_line_fingertip.points.resize(cloud_line_fingertip.width * cloud_line_fingertip.height);

					for(int t=0;t<cloud_line_fingertip.points.size();t++){
						cloud_line_fingertip.points[t].x=x_Nose+(x_Fingertip-x_Nose)*t/100;
						cloud_line_fingertip.points[t].y=y_Nose+(y_Fingertip-y_Nose)*t/100;
						cloud_line_fingertip.points[t].z=z_Nose+(z_Fingertip-z_Nose)*t/100;
					}
				}
				if(dis_First_joint<=0.2&&z_Nose!=0.0){
					cloud_line_first.width = 1000;
					cloud_line_first.height = 1;
					cloud_line_first.points.resize(cloud_line_first.width * cloud_line_first.height);
					for(int t=0;t<cloud_line_first.points.size();t++){
						cloud_line_first.points[t].x=x_Nose+(x_First_joint-x_Nose)*t/100;
						cloud_line_first.points[t].y=y_Nose+(y_First_joint-y_Nose)*t/100;
						cloud_line_first.points[t].z=z_Nose+(z_First_joint-z_Nose)*t/100;
					}
				}
				if(dis_Second_joint<=0.2&&z_Nose!=0.0){
					cloud_line_second.width = 1000;
					cloud_line_second.height = 1;
					cloud_line_second.points.resize(cloud_line_second.width * cloud_line_second.height);
					for(int t=0;t<cloud_line_second.points.size();t++){
						cloud_line_second.points[t].x=x_Nose+(x_Second_joint-x_Nose)*t/100;
						cloud_line_second.points[t].y=y_Nose+(y_Second_joint-y_Nose)*t/100;
						cloud_line_second.points[t].z=z_Nose+(z_Second_joint-z_Nose)*t/100;
					}
				}
				if(dis_Third_joint<=0.2&&z_Nose!=0.0){
					cloud_line_third.width = 1000;
					cloud_line_third.height = 1;
					cloud_line_third.points.resize(cloud_line_third.width * cloud_line_third.height);
					for(int t=0;t<cloud_line_third.points.size();t++){
						cloud_line_third.points[t].x=x_Nose+(x_Third_joint-x_Nose)*t/100;
						cloud_line_third.points[t].y=y_Nose+(y_Third_joint-y_Nose)*t/100;
						cloud_line_third.points[t].z=z_Nose+(z_Third_joint-z_Nose)*t/100;
					}
				}

			}else{
				std::cout << "1_Not_Calibration" << '\n';
			}

		//2回目のキャリブレーション=========================================================================================================
		//仰角の基準となる関節の補正値を求める
		pcl::PointCloud<PointT> cloud_hand;
		Eigen::Vector4f estimation_tip,estimation_first,estimation_second,estimation_third;
		double dis_tip,dis_first,dis_second,dis_third=DBL_MAX;
		double dis_min=DBL_MAX;

		if(flag_line==2){
			//較正と同時にmap基準に変換されている
			//指先の仰角による較正===========================================================================================================
			if(dis_Fingertip<=0.2&&z_Nose!=0.0){
				cloud_calibration_fingertip=Calibration_angle(transform_line_after , x_Nose , y_Nose , z_Nose, x_Fingertip , y_Fingertip , z_Fingertip, rad_fingertip);
			}
			//第一関節の仰角による較正
			if(dis_First_joint<=0.2&&z_Nose!=0.0){
				cloud_calibration_first=Calibration_angle(transform_line_after , x_Nose , y_Nose , z_Nose, x_First_joint , y_First_joint , z_First_joint, rad_first);
			}
			//第二関節の仰角による較正
			if(dis_Second_joint<=0.2&&z_Nose!=0.0){
				cloud_calibration_second=Calibration_angle(transform_line_after , x_Nose , y_Nose , z_Nose, x_Second_joint , y_Second_joint , z_Second_joint, rad_second);
			}
			//第三関節の仰角による較正
			if(dis_Third_joint<=0.2&&z_Nose!=0.0){
				cloud_calibration_third=Calibration_angle(transform_line_after , x_Nose , y_Nose , z_Nose, x_Third_joint , y_Third_joint , z_Third_joint, rad_third);
			}

			//基準点となる関節の特定===========================================================================================================
			//ハンドの位置をmap基準に変換する
			cloud_hand.width = 1;
	    cloud_hand.height = 1;
	    cloud_hand.points.resize(cloud_hand.width * cloud_hand.height);

	 		cloud_hand.points[0].x=(float)vector_palm.getX();
	 		cloud_hand.points[0].y=(float)vector_palm.getY();
	 		cloud_hand.points[0].z=(float)vector_palm.getZ();
		 pcl::PointCloud<PointT> cloud_output_hand;
		 sensor_msgs::PointCloud2 conv_cloud_msg_hand;
		 sensor_msgs::PointCloud2 cloud_msg_hand;
		 pcl::toROSMsg(cloud_hand, cloud_msg_hand);
		 cloud_msg_hand.header.frame_id = "head_rgbd_sensor_rgb_frame";
		 pcl::PointCloud<PointT>::Ptr cloud_input_hand(new pcl::PointCloud<PointT>);
		 std::vector<int> index_all_hand;
		 Eigen::Matrix4f mat_hand = tf2::transformToEigen(transform_line_after.transform).matrix().cast<float>();
		 pcl_ros::transformPointCloud(mat_hand,cloud_msg_hand, conv_cloud_msg_hand);
		 pcl::fromROSMsg(conv_cloud_msg_hand, *cloud_input_hand);                                         //msg→pointcloud
		 pcl::removeNaNFromPointCloud(*cloud_input_hand,cloud_output_hand,index_all_hand);
		 double z_hand=cloud_output_hand.points[0].z;
		 //map基準のhandの位置
		 std::cout << "x_hand=" << cloud_output_hand.points[0].x<<'\n';
		 std::cout << "y_hand=" << cloud_output_hand.points[0].y<<'\n';
		 std::cout << "z_hand=" << cloud_output_hand.points[0].z<<'\n';
		 std::cout  <<'\n';
		 //handに指した各基準点較正後の推定位置
		 if(cloud_calibration_fingertip.size()>0){
			 estimation_tip=Estimate_point(cloud_calibration_fingertip,z_hand);
			 std::cout << "estimation_tip.x()" << estimation_tip.x()<<'\n';
 			 std::cout << "estimation_tip.y()" << estimation_tip.y()<<'\n';
 			 std::cout << "estimation_tip.z()" << estimation_tip.z()<<'\n';
			 std::cout  <<'\n';
 			 dis_tip=sqrt(pow(estimation_tip.x(),2.0)+pow(estimation_tip.y(),2.0));
		 }
		 if(cloud_calibration_first.size()>0){
			 estimation_first=Estimate_point(cloud_calibration_first,z_hand);
			 std::cout << "estimation_first.x()" << estimation_first.x()<<'\n';
 			 std::cout << "estimation_first.y()" << estimation_first.y()<<'\n';
 			 std::cout << "estimation_first.z()" << estimation_first.z()<<'\n';
			 std::cout  <<'\n';
			 dis_first=sqrt(pow(estimation_first.x(),2.0)+pow(estimation_first.y(),2.0));
		 }
		 if(cloud_calibration_second.size()>0){
			 estimation_second=Estimate_point(cloud_calibration_second,z_hand);
			 std::cout << "estimation_second.x()" << estimation_second.x()<<'\n';
 			 std::cout << "estimation_second.y()" << estimation_second.y()<<'\n';
 			 std::cout << "estimation_second.z()" << estimation_second.z()<<'\n';
			 std::cout  <<'\n';
			 dis_second=sqrt(pow(estimation_second.x(),2.0)+pow(estimation_second.y(),2.0));
		 }
		 if(cloud_calibration_third.size()>0){
			 estimation_third=Estimate_point(cloud_calibration_third,z_hand);
			 std::cout << "estimation_third.x()" << estimation_third.x()<<'\n';
 			 std::cout << "estimation_third.y()" << estimation_third.y()<<'\n';
 			 std::cout << "estimation_third.z()" << estimation_third.z()<<'\n';
			 std::cout  <<'\n';
			 dis_third=sqrt(pow(estimation_third.x(),2.0)+pow(estimation_third.y(),2.0));
		 }
		 //ｘ−ｙ平面のhandまでの距離の最小で基準となる関節を決める
		 std::vector<double> dis={dis_tip,dis_first,dis_second,dis_third};
		 for(int i=0;i<dis.size();i++){
			 std::cout << "dis[i]" << dis[i]<<'\n';
			if(dis[i]<dis_min){
				dis_min=dis[i];
				min_number=i;
			}
		}
		switch (min_number)
		{
		 case 0:
			 rad_best=rad_fingertip;
			 std::cout << "Fingertip is reference point" << '\n';
			 break;
		 case 1:
			 rad_best=rad_first;
			 std::cout << "First_joint is reference point" << '\n';
			 break;
		 case 2:
			 rad_best=rad_second;
			 std::cout << "Second_joint is reference point" << '\n';
			 break;
		 case 3:
			 rad_best=rad_third;
			 std::cout << "Third_joint is reference point" << '\n';
			 break;
		default:
			break;
		 }
		std::cout << "rad_best=" <<rad_best<< '\n';
		}else{
			std::cout << "2_Not_Calibration" << '\n';
		}
		//==================================Calibrationの初期設定終了==========================


		//指差しの認識する。認識後、基準点をもとに較正を行う===============================================================================
		if(flag_line==3){
			switch (min_number)
			{
			 case 0:
				 dis_best=dis_Fingertip;x_best=x_Fingertip;y_best=y_Fingertip;z_best=z_Fingertip;
				 break;
			 case 1:
				 dis_best=dis_First_joint;x_best=x_First_joint;y_best=y_First_joint;z_best=z_First_joint;
				 break;
			 case 2:
				 dis_best=dis_Second_joint;x_best=x_Second_joint;y_best=y_Second_joint;z_best=z_Second_joint;
				 break;
			 case 3:
				 dis_best=dis_Third_joint;x_best=x_Third_joint;y_best=y_Third_joint;z_best=z_Third_joint;
				 break;
			default:
				break;
			 }
			 if(dis_best<=0.2&&z_Nose!=0.0){
				 cloud_line_destination.width = 1000;
				 cloud_line_destination.height = 1;
				 cloud_line_destination.points.resize(cloud_line_destination.width * cloud_line_destination.height);

				for(int t=0;t<cloud_line_destination.points.size();t++){
					cloud_line_destination.points[t].x=x_Nose+(x_best-x_Nose)*t/100;
					cloud_line_destination.points[t].y=y_Nose+(y_best-y_Nose)*t/100;
					cloud_line_destination.points[t].z=z_Nose+(z_best-z_Nose)*t/100;
				}
			}
				cloud_calibration_destination=Calibration_angle(transform_line_after , x_Nose , y_Nose , z_Nose, x_best , y_best , z_best, rad_best);
			}else{
				std::cout << "3_Not_Calibration" << '\n';
			}



		pcl::PointCloud<PointT>::Ptr cloud_dwnsmp(new pcl::PointCloud<PointT>);
    //ダウンサンプリングを行う
		cloud_dwnsmp = Down_Sampling(cloud_trimming);

		pcl::PointCloud<PointT>::Ptr cloud_rest(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_plane(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_table_rest(new pcl::PointCloud<PointT>);
		pcl::PointCloud<PointT>::Ptr cloud_table_plane(new pcl::PointCloud<PointT>);


		//較正後の指差し方向から指差し推定位置を算出=================================================================================
		if(flag_line==4){
			//卓上認識========================
			// 床平面を認識する。================
			pcl::SACSegmentation<PointT> seg;
			pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
			pcl::ModelCoefficients::Ptr coefficients (new pcl::ModelCoefficients);
			seg.setOptimizeCoefficients(true);
			seg.setModelType(pcl::SACMODEL_PLANE);
			seg.setMethodType(pcl::SAC_RANSAC);
			seg.setMaxIterations (100);  //試行回数
			seg.setDistanceThreshold(threshold_plane);//閾値設定
			seg.setInputCloud(cloud_dwnsmp);
			seg.segment(*inliers, *coefficients);

			// 床平面を除去する==================
			pcl::ExtractIndices<PointT> extract;
			extract.setInputCloud(cloud_dwnsmp);//入力データ
			extract.setIndices(inliers);//インデクッスの入力
			extract.setNegative(false);//平面以外を除去
			extract.filter(*cloud_plane);
			extract.setNegative(true);//平面を除去
			extract.filter(*cloud_rest);

			// 机平面を認識する。===================================================================================
			pcl::SACSegmentation<PointT> seg2;
			pcl::PointIndices::Ptr inliers2(new pcl::PointIndices);
			pcl::ModelCoefficients::Ptr coefficients2 (new pcl::ModelCoefficients);
			seg2.setOptimizeCoefficients(true);
			seg2.setModelType(pcl::SACMODEL_PLANE);
			seg2.setMethodType(pcl::SAC_RANSAC);
			seg2.setMaxIterations (100);  //試行回数
			seg2.setDistanceThreshold(threshold_plane);//閾値設定
			seg2.setInputCloud(cloud_rest);
			seg2.segment(*inliers2, *coefficients2);

			// 机平面を除去する==================
			pcl::ExtractIndices<PointT> extract2;
			extract2.setInputCloud(cloud_rest);//入力データ
			extract2.setIndices(inliers2);//インデクッスの入力
			extract2.setNegative(false);//平面以外を除去
			extract2.filter(*cloud_table_plane);
			extract2.setNegative(true);//平面を除去
			extract2.filter(*cloud_table_rest);

			//机平面にtfをはる=======================================================================
			geometry_msgs::PointStamped msgs_plane_ave_point;
			Eigen::Vector4f plane_ave_point = ave_point_of_PointCloud(cloud_table_plane,pub_plane_ave_point,msgs_plane_ave_point);
			tf2::Transform transform_plane;
			tf2::Vector3 plane_axis;//ベクトルの生成
			tf2::Vector3 z_axis(0, 0, 1);//z軸
			float a_plane = coefficients2->values[0];//平面のクラスタの点群の情報
			float b_plane = coefficients2->values[1];
			float c_plane = coefficients2->values[2];
			if(a_plane <= 0)//床平面の軸が上側にあるとき
			{
				plane_axis.setValue(-1*a_plane, -1*b_plane, -1*c_plane);
			}
			else//床平面の軸が上側にあるとき
			{
				plane_axis.setValue(a_plane, b_plane, c_plane);
			}
			//	plane_axis.setValue(a_plane, b_plane, c_plane);
			plane_axis = plane_axis.normalized(); //ベクトルの正規化（ベクトルの方向を維持しつつ長さを１にする）

			double plane_angle = plane_axis.angle(z_axis);
			//回転軸
			tf2::Vector3 plane2_axis(z_axis.y()*plane_axis.z() - z_axis.z()*plane_axis.y(),z_axis.z()*plane_axis.x() - z_axis.x()*plane_axis.z() , z_axis.x()*plane_axis.y() - z_axis.y()*plane_axis.x());
			//外積：法線だす
			plane2_axis = plane2_axis.normalized();

			transform_plane.setOrigin( tf2::Vector3(plane_ave_point.x(), plane_ave_point.y(), plane_ave_point.z()) );//位置
			transform_plane.setRotation( tf2::Quaternion(plane2_axis, plane_angle) );//姿勢
			geometry_msgs::TransformStamped plane_transformStamped;
			plane_transformStamped.header.stamp = ros::Time::now();
			plane_transformStamped.header.frame_id = "head_rgbd_sensor_rgb_frame";
			plane_transformStamped.child_frame_id = "apu/plane";

			plane_transformStamped.transform.translation.x = transform_plane.getOrigin().x();
			plane_transformStamped.transform.translation.y = transform_plane.getOrigin().y();
			plane_transformStamped.transform.translation.z = transform_plane.getOrigin().z();

			plane_transformStamped.transform.rotation.x = transform_plane.getRotation().x();
			plane_transformStamped.transform.rotation.y = transform_plane.getRotation().y();
			plane_transformStamped.transform.rotation.z = transform_plane.getRotation().z();
			plane_transformStamped.transform.rotation.w = transform_plane.getRotation().w();
			br_plane.sendTransform(plane_transformStamped);


		//較正していない指差し方向============================================================
			pcl::PointCloud<PointT>::Ptr cloud_transform_line(new pcl::PointCloud<PointT>);
			sensor_msgs::PointCloud2 cloud_msg_line;
			sensor_msgs::PointCloud2 conv_cloud_msg;
			pcl::PointCloud<PointT>::Ptr cloud_input_line(new pcl::PointCloud<PointT>);
			std::vector<int> index_all_line;
			pcl::toROSMsg(cloud_line_destination, cloud_msg_line);
			cloud_msg_line.header.frame_id = "head_rgbd_sensor_rgb_frame";
		if(cloud_line_destination.size()>0){
		//姿勢をmapに合わせる===================================================================
			//座標変換
			Eigen::Matrix4f mat = tf2::transformToEigen(transform_line_after.transform).matrix().cast<float>();
			pcl_ros::transformPointCloud(mat,cloud_msg_line, conv_cloud_msg);
			//msg→pointcloud
			pcl::fromROSMsg(conv_cloud_msg, *cloud_input_line);
			pcl::removeNaNFromPointCloud(*cloud_input_line,cloud_line_destination,index_all_line);
		}


	 //estimate position prepare==================================================================================
	 //cloud_table_plane
	 pcl::PointCloud<PointT>::Ptr cloud_transform_table_plane(new pcl::PointCloud<PointT>);
	 sensor_msgs::PointCloud2 cloud_msg_table_plane;
	 sensor_msgs::PointCloud2 conv2_cloud_msg;
	 pcl::PointCloud<PointT>::Ptr cloud_input_table_plane(new pcl::PointCloud<PointT>);
	 std::vector<int> index_all_table_plane;
	 pcl::PointCloud<PointT>::Ptr cloud_trimming_table_plane(new pcl::PointCloud<PointT>);
	 pcl::toROSMsg(*cloud_table_plane, cloud_msg_table_plane);
	 cloud_msg_table_plane.header.frame_id = "head_rgbd_sensor_rgb_frame";

	 //cloud_table_rest
	 pcl::PointCloud<PointT>::Ptr cloud_transform_rest(new pcl::PointCloud<PointT>);
	 sensor_msgs::PointCloud2 cloud_msg_rest;
	 sensor_msgs::PointCloud2 conv3_cloud_msg;
	 pcl::PointCloud<PointT>::Ptr cloud_input_rest(new pcl::PointCloud<PointT>);
	 std::vector<int> index_all_rest;
	 pcl::PointCloud<PointT>::Ptr cloud_trimming_table_rest(new pcl::PointCloud<PointT>);
	 pcl::toROSMsg(*cloud_table_rest, cloud_msg_rest);
	 cloud_msg_rest.header.frame_id = "head_rgbd_sensor_rgb_frame";

	 Eigen::Vector4f centroid_point_table_plane;
	 Eigen::Vector4f estimation_point;
	 Eigen::Vector4f estimation_point_before;

	  //推定位置の算出==================================================================================
	 if(cloud_table_plane->size()>0){
		 //map基準に変換
		 tf2_ros::TransformListener tf_listener(tfBuffer);
			geometry_msgs::TransformStamped transform_table_plane;
			if(tfBuffer.canTransform("map", "head_rgbd_sensor_rgb_frame", ros::Time::now(), ros::Duration(10.0))){//tfにおけるwaitForTransformの代わり,変換できていれば１を返す
				try{
				 transform_table_plane=tfBuffer.lookupTransform("map", "head_rgbd_sensor_rgb_frame", ros::Time(0));
					}
				catch (tf2::TransformException& ex){
						ROS_ERROR("TF Exception: %s", ex.what());
						ros::Duration(1.0).sleep();
					}
				}
			//Transform_plane
			Eigen::Matrix4f mat_table_plane = tf2::transformToEigen(transform_table_plane.transform).matrix().cast<float>();
			pcl_ros::transformPointCloud(mat_table_plane,cloud_msg_table_plane, conv2_cloud_msg);
			pcl::fromROSMsg(conv2_cloud_msg, *cloud_input_table_plane);
			pcl::removeNaNFromPointCloud(*cloud_input_table_plane,*cloud_trimming_table_plane,index_all_table_plane);

			//Transform_rest
			Eigen::Matrix4f mat_rest = tf2::transformToEigen(transform_table_plane.transform).matrix().cast<float>();
			pcl_ros::transformPointCloud(mat_rest,cloud_msg_rest, conv3_cloud_msg);
			pcl::fromROSMsg(conv3_cloud_msg, *cloud_input_rest);
			pcl::removeNaNFromPointCloud(*cloud_input_rest,*cloud_trimming_table_rest,index_all_rest);

			pcl::compute3DCentroid(*cloud_trimming_table_plane, centroid_point_table_plane);//planeの重心を計算

			//通常
			for(int i=0;i<cloud_line_destination.size();i++){
				if(cloud_line_destination.points[i].z >=centroid_point_table_plane.z()-0.005&&cloud_line_destination.points[i].z <=centroid_point_table_plane.z()+0.005){
					estimation_point_before.x()=cloud_line_destination.points[i].x;
					estimation_point_before.y()=cloud_line_destination.points[i].y;
					estimation_point_before.z()=cloud_line_destination.points[i].z;
				}
			}
			//通常の指差し方向の較正結果============================================
			std::cout << "/////////////estimation_point_before.x()" << estimation_point_before.x()<<'\n';
			std::cout << "/////////////estimation_point_before.y()" << estimation_point_before.y()<<'\n';
			std::cout << "/////////////estimation_point_before.z()" << estimation_point_before.z()<<'\n';

			//較正
			for(int i=0;i<cloud_calibration_destination.size();i++){
				if(cloud_calibration_destination.points[i].z >=centroid_point_table_plane.z()-0.005&&cloud_calibration_destination.points[i].z <=centroid_point_table_plane.z()+0.005){
					estimation_point.x()=cloud_calibration_destination.points[i].x;
					estimation_point.y()=cloud_calibration_destination.points[i].y;
					estimation_point.z()=cloud_calibration_destination.points[i].z;
				}
			}
			//最終的な屈曲を考慮した指差し方向の較正結果============================================
			std::cout << "/////////////estimation_point.x()" << estimation_point.x()<<'\n';
			std::cout << "/////////////estimation_point.y()" << estimation_point.y()<<'\n';
			std::cout << "/////////////estimation_point.z()" << estimation_point.z()<<'\n';
		}
	}else{
		std::cout << "4_Not_Calibration" << '\n';
	}
	std::cout << "" << '\n';

		// 出力================================================================
		if(cloud_Nose->size() > 0)
		{
			sensor_msgs::PointCloud2 msg_Nose;
			Output_pub(cloud_Nose,msg_Nose,pub_Nose);
		}
		else
		{
			ROS_INFO("Size (cloud_Nose)");
		}
		// if(cloud_Wrist->size() > 0)
		// {
		// 	sensor_msgs::PointCloud2 msg_Wrist;
		// 	Output_pub(cloud_trimming_line,msg_Wrist,pub_Wrist);
		// }
		// else
		// {
		// 	ROS_INFO("Size (cloud_Wrist)");
		// }
		if(cloud_Fingertip->size() > 0)
		{
			sensor_msgs::PointCloud2 msg_Fingertip;
			Output_pub(cloud_Fingertip,msg_Fingertip,pub_Fingertip);
		}
		else
		{
			ROS_INFO("Size (cloud_Fingertip)");
		}
		if(cloud_First_joint->size() > 0)
		{
			sensor_msgs::PointCloud2 msg_First_joint;
			Output_pub(cloud_First_joint,msg_First_joint,pub_First_joint);
		}
		else
		{
			ROS_INFO("Size (cloud_first_joint)");
		}
		if(cloud_Second_joint->size() > 0)
		{
			sensor_msgs::PointCloud2 msg_Second_joint;
			Output_pub(cloud_Second_joint,msg_Second_joint,pub_Second_joint);
		}
		else
		{
			ROS_INFO("Size (cloud_Second_joint)");
		}
		if(cloud_Third_joint->size() > 0)
		{
			sensor_msgs::PointCloud2 msg_Third_joint;
			Output_pub(cloud_Third_joint,msg_Third_joint,pub_Third_joint);
		}
		else
		{
			ROS_INFO("Size (cloud_Third_joint)");
		}
		if(cloud_plane->size() > 0)
		{
			sensor_msgs::PointCloud2 msg_plane;
			Output_pub(cloud_plane,msg_plane,pub_plane);
		}
		else
		{
			ROS_INFO("Size (cloud_plane)");
		}
		if(cloud_table_plane->size() > 0)
		{
			sensor_msgs::PointCloud2 msg_table_plane;
			Output_pub(cloud_table_plane,msg_table_plane,pub_table_plane);
		}
		else
		{
			ROS_INFO("Size (cloud_table_plane)");
		}
		if(cloud_line_fingertip.size() > 0)
		{
			sensor_msgs::PointCloud2 msg_line_fingertip;
			Output_pub_raw(cloud_line_first,msg_line_fingertip,pub_line_fingertip);
		}
		else
		{
			ROS_INFO("Size (cloud_line_fingertip)");
		}
		if(cloud_line_first.size() > 0)
		{
			sensor_msgs::PointCloud2 msg_line_first;
			// Output_pub_raw(cloud_line_first,msg_line_first,pub_line_first);
			Output_pub_map(cloud_calibration_first,msg_line_first,pub_line_first);
		}
		else
		{
			ROS_INFO("Size (cloud_line_first)");
		}
		if(cloud_line_second.size() > 0)
		{
			sensor_msgs::PointCloud2 msg_line_second;
			Output_pub_raw(cloud_calibration_second,msg_line_second,pub_line_second);
		}
		else
		{
			ROS_INFO("Size (cloud_line_second)");
		}
		if(cloud_line_third.size() > 0)
		{
			sensor_msgs::PointCloud2 msg_line_third;
			Output_pub_raw(cloud_calibration_third,msg_line_third,pub_line_third);
		}
		else
		{
			ROS_INFO("Size (cloud_line_third)");
		}
		if(cloud_line_destination.size() > 0)
		{
			sensor_msgs::PointCloud2 msg_line_destination;
			Output_pub_raw(cloud_line_destination,msg_line_destination,pub_line_destination);
		}
		else
		{
			ROS_INFO("Size (cloud_line_destination)");
		}
		if(cloud_calibration_destination.size() > 0)
		{
			sensor_msgs::PointCloud2 msg_calibration_destination;
			Output_pub_map(cloud_calibration_destination,msg_calibration_destination,pub_calibration_destination);
		}
		else
		{
			ROS_INFO("Size (cloud_calibration_destination)");
		}

		std::cout << "calibration_pointing_end" << '\n';
		return;
	}
};

int main(int argc, char** argv)
{
	ros::init (argc, argv, "calibration_pointing");

	calibration_pointing calibration_pointing;

	ros::Rate spin_rate(1);
	while(ros::ok())
	{
		ros::spinOnce();
		spin_rate.sleep();
	}

	return 0;
}
