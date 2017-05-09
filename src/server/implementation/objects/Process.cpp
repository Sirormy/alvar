#include "Process.h"

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "unistd.h"
#include "sys/wait.h"

#include "MarkerDetector.h"
//#define USE_URL_WGET
#define USE_URL_SOUP
#ifdef USE_URL_SOUP
#include <libsoup/soup.h>
#endif

void addFgWithAlpha(cv::Mat &bg, cv::Mat &fg) {
  if (fg.channels() < 4) {
    fg.copyTo(bg);  
    return;
  }
  std::vector<cv::Mat> splitted_bg, splitted_fg;
  cv::split(bg, splitted_bg);
  cv::split(fg, splitted_fg);
  cv::Mat mask = splitted_fg[3];
  cv::Mat invmask = ~mask;
  splitted_bg[0] = (splitted_bg[0] - mask) + (splitted_fg[0] - invmask);
  splitted_bg[1] = (splitted_bg[1] - mask) + (splitted_fg[1] - invmask);
  splitted_bg[2] = (splitted_bg[2] - mask) + (splitted_fg[2] - invmask);
  if (bg.channels() > 3) {
    splitted_bg[3] = splitted_bg[3] + splitted_fg[3];
  }
  cv::merge(splitted_bg, bg);
}

#ifdef USE_URL_SOUP
bool is_valid_uri(std::string uri) {
  // TODO
  return true;
}

bool load_from_url(const char *file_name, const char *url) {
  SoupSession *session;
  SoupMessage *msg;
  FILE *dst;
  session = soup_session_sync_new ();
  msg = soup_message_new ("GET", url);
  if (msg == NULL){
    goto end2;
  }
  soup_session_send_message (session, msg);
  dst = fopen (file_name, "w+");
  if (dst == NULL) {
    goto end;
  }
  fwrite (msg->response_body->data, 1, msg->response_body->length, dst);
  fclose (dst);
end:
  g_object_unref (msg);
end2:
  g_object_unref (session);
}
#endif

cv::Mat ArProcess::readImage(std::string uri) {
  cv::Mat bg;

#ifdef USE_URL_SOUP
  bg = cv::imread(uri, CV_LOAD_IMAGE_UNCHANGED);
  if ((!bg.data) && is_valid_uri(uri)) {
    std::string tmpfile("/tmp/tmp.png");
    std::cout<<"SOUP: Loading URL image to temp file: "<<tmpfile<<std::endl;
    load_from_url (tmpfile.c_str(), uri.c_str());
    bg = cv::imread(tmpfile, CV_LOAD_IMAGE_UNCHANGED);
  }
#endif

#ifdef USE_URL_WGET
  std::string tmpfile("/tmp/tmp.png"); //tmpnam(NULL));
  pid_t pid = fork();
  if (pid == 0) {
    std::cout<<"WGET: Loading URL image to temp file: "<<tmpfile<<std::endl;
    //TODO: Why this does not work: http://www.fnordware.com/superpng/straight.png
    //http://www.dplkbumiputera.com/slider_image/sym/root/proc/self/cwd/usr/share/zenity/clothes/monk.png
    //http://www.dplkbumiputera.com/slider_image/sym/root/proc/self/cwd/usr/share/zenity/clothes/hawaii-shirt.png
    //execlp("/usr/bin/wget", "/usr/bin/wget", "-O", "/tmp/tmp.png", "http://www.fnordware.com/superpng/straight.png", NULL);
    //execlp("/usr/bin/wget", "/usr/bin/wget", "-O", "/tmp/tmp.png", "http://www.dplkbumiputera.com/slider_image/sym/root/proc/self/cwd/usr/share/zenity/clothes/hawaii-shirt.png", NULL);
    //execlp("/usr/bin/wget", "/usr/bin/wget", "-O", "/tmp/tmp.png", "http://www.dplkbumiputera.com/slider_image/sym/root/proc/self/cwd/usr/share/zenity/clothes/sunglasses.png", NULL);
    execlp("/usr/bin/wget", "/usr/bin/wget", "-O", tmpfile.c_str(), uri.c_str(), NULL);
    printf("\nError: Could not execute wget\n");
    _exit(0);
  } else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
  }
  bg = cv::imread(tmpfile, CV_LOAD_IMAGE_UNCHANGED);
#endif

#ifdef USE_URL_AS_FILE
  bg = cv::imread(uri, CV_LOAD_IMAGE_UNCHANGED);
#endif

  if (!bg.data) {
    bg = cv::Mat(256,256,CV_8UC3);
  }
  if (bg.channels() == 3) {
    cv::cvtColor(bg, bg, CV_BGR2BGRA);
  }
  std::cout<<"BG image: "<<bg.cols<<"x"<<bg.rows<<" channels: "<<bg.channels()<<std::endl;
  return bg;
}

ArProcess::ArProcess() : overlayScale(1.f), owndata(0), mShowDebugLevel(1) {
  pthread_mutex_init(&mMutex, NULL);
  owndata = new alvar::MarkerDetector<alvar::MarkerArtoolkit>();
  marker_size = 2.0;
  alvar::MarkerDetector<alvar::MarkerArtoolkit> &marker_detector = 
    *(alvar::MarkerDetector<alvar::MarkerArtoolkit> *)owndata;
  marker_detector.SetMarkerSize(marker_size,3,1.5);
  std::vector<int> id_vector;
  //number of markers
  for(int i = 0; i < 5; ++i)
    id_vector.push_back(i);
  multi_marker = new alvar::MultiMarker(id_vector);
  //set pose relation between markers.
  pose.Reset();
  multi_marker->PointCloudAdd(0, marker_size, pose);
  pose.SetTranslation(marker_size*1.75, 0, 0);
  multi_marker->PointCloudAdd(1, marker_size, pose);
  pose.SetTranslation(+marker_size*3.5, 0, 0);
  multi_marker->PointCloudAdd(2, marker_size, pose);
  pose.SetTranslation(+marker_size*5.25, 0, 0);
  multi_marker->PointCloudAdd(3, marker_size, pose);
  pose.SetTranslation(+marker_size*7.0, 0, 0);
  multi_marker->PointCloudAdd(4, marker_size, pose);
}

ArProcess::~ArProcess() {
  if (owndata) delete (alvar::MarkerDetector<alvar::MarkerArtoolkit> *)owndata;
  pthread_mutex_destroy(&mMutex);
}

float ArProcess::set_overlay_scale(float _overlayScale) {
  if (_overlayScale > 0.f) overlayScale=_overlayScale;
  else overlayScale=1.f;
  return overlayScale;
}

bool ArProcess::set_overlay(const char *_overlay_image, const char *_overlay_text) {
  pthread_mutex_lock(&mMutex);
  cv::Mat fg, bg;
  if (_overlay_image) overlay_image = std::string(_overlay_image);
  if (_overlay_text) overlay_text = std::string(_overlay_text);

  if (overlay_image.length() > 0) {
    bg = readImage(overlay_image);
  }
  if (overlay_text.length() > 0) {
    int font = cv::FONT_HERSHEY_PLAIN, font_thickness = 3;
    double font_scale = 6.0;
    int baseline=0;
    cv::Size textSize = cv::getTextSize(overlay_text.c_str(), font, font_scale, font_thickness, &baseline);
    fg = cv::Mat(textSize.height+baseline+4, textSize.width+4, CV_8UC4); // TODO: CV_8UC4
    fg.setTo(cv::Scalar(255,255,255,64));
    cv::putText(fg, overlay_text.c_str(), cv::Point(2, fg.rows-(baseline/2)), font, font_scale, cv::Scalar(0,64,0,255), font_thickness);
  }
  int res = 0;
  if (bg.data) res = std::max(bg.cols, bg.rows);
  if (fg.data) res = std::max(res, std::max(fg.cols, fg.rows));
  if (res < 1) return -1;
  overlay = cv::Mat(res, res, CV_8UC4);
  overlay.setTo(cv::Scalar(255,255,255,0));
  if (bg.data) {
    double fx = res/std::max(bg.cols, bg.rows), fy = fx;
    cv::resize(bg, bg, cv::Size(), fx, fy);
    cv::Mat overlay_bg(overlay, cv::Rect(
      (overlay.cols-bg.cols)/2, (overlay.rows-bg.rows)/2,
      bg.cols, bg.rows));
    bg.copyTo(overlay_bg);
  }
  if (fg.data) {
    double fx = res/std::max(fg.cols, fg.rows), fy = fx;
    cv::resize(fg, fg, cv::Size(), fx, fy);
    cv::Mat overlay_fg(overlay, cv::Rect(
      (overlay.cols-fg.cols)/2, (overlay.rows-fg.rows)/2,
      fg.cols, fg.rows));
    addFgWithAlpha(overlay_fg, fg);
  }
  pthread_mutex_unlock(&mMutex);
  return true;
}

int ArProcess::detect_marker(IplImage *image) {
  pthread_mutex_lock(&mMutex);
  alvar::Camera cam;
  double error = 1000;
  std::cout<<"image width:"<<image->width<<"image height:"<<image->height<<std::endl;
  cam.SetRes(image->width, image->height);
  alvar::MarkerDetector<alvar::MarkerArtoolkit> &marker_detector = 
    *(alvar::MarkerDetector<alvar::MarkerArtoolkit> *)owndata;
  marker_detector.SetMarkerSize(marker_size,3,1.5);
  if(marker_detector.Detect(image, &cam, true, (mShowDebugLevel > 0)))
    error = multi_marker->Update(marker_detector.markers, &cam, pose);

#if 1
  detectedMarkersPrev = detectedMarkers;
  std::map<int,int>::iterator iter;
  for (iter = detectedMarkers.begin(); iter != detectedMarkers.end(); iter++) {
    iter->second = 0; // Reset counters (but do not forget that this was seen previously?)
  }

  if (!overlay.data) {
    pthread_mutex_unlock(&mMutex);
    return 0;
  }

  for (size_t i=0; i<marker_detector.markers->size(); i++) {
    if (i >= 32) break;

    int marker_id = (*(marker_detector.markers))[i].GetId();
    detectedMarkers[marker_id]++;
    std::cout<<"marker_id:"<<marker_id<<std::endl;
  }

  std::cout<<"multi marker error is:"<<error<<std::endl;



  if((error >= 0)&&(error < 5)&&marker_detector.markers->size()>0){
    cv::Mat warped_overlay(image->height, image->width, CV_8UC4);
    warped_overlay.setTo(cv::Scalar(0,0,0,0));
    std::cout<<"overlayScale is:"<<overlayScale<<std::endl;

    if (overlayScale > 0.5f) {
      cv::Point2f source_points[4];
      cv::Point2f dest_points[4];
      float overlay_maxdim = std::max(overlay.rows, overlay.cols);
      overlay_maxdim /= overlayScale;
      source_points[0] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(-overlay_maxdim/2,overlay_maxdim/2);
      source_points[1] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(overlay_maxdim/2,overlay_maxdim/2);
      source_points[2] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(overlay_maxdim/2,-overlay_maxdim/2);
      source_points[3] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(-overlay_maxdim/2,-overlay_maxdim/2);
      //source_points[0] = cv::Point2f(0,overlay.rows-1);
      //source_points[1] = cv::Point2f(overlay.cols-1,overlay.rows-1);
      //source_points[2] = cv::Point2f(overlay.cols-1,0);
      //source_points[3] = cv::Point2f(0,0);
      
      double visualize3d_points[4][3] = {
	// square
	{ -(marker_size/2), -(marker_size/2), marker_size },
	{ -(marker_size/2),  (marker_size/2), marker_size },
	{  (marker_size/2),  (marker_size/2), marker_size },
	{  (marker_size/2), -(marker_size/2), marker_size },
      };

      double visualize2d_points[4][2];
      
      CvMat visualize3d_points_mat;
      CvMat visualize2d_points_mat;
      cvInitMatHeader(&visualize3d_points_mat,4,3,CV_64F,visualize3d_points);
      cvInitMatHeader(&visualize2d_points_mat,4,2,CV_64F,visualize2d_points);
      cam.ProjectPoints(&visualize3d_points_mat, &pose, &visualize2d_points_mat);
      
     for(int i = 0; i != 4; i++){
       dest_points[i] = cv::Point2f(visualize2d_points[i][0],visualize2d_points[i][1]);
       std::cout<<"x:"<<visualize2d_points[i][0]<<"y:"<<visualize2d_points[i][1]<<std::endl;
      }
        
      for(int i = 0;i != 4; i++){
	/*
        dest_points[i] = cv::Point2f(
        (*(marker_detector.markers))[0].marker_corners_img[i].x,
        (*(marker_detector.markers))[0].marker_corners_img[i].y);
	*/
      std::cout<<"image x:"<<(*(marker_detector.markers))[0].marker_corners_img[i].x<<"image y:"<<(*(marker_detector.markers))[0].marker_corners_img[i].y<<std::endl;
}
     
      std::cout<<"image width:"<<image->width<<"image height:"<<image->height<<std::endl;
      cv::Mat transform = cv::getPerspectiveTransform(source_points, dest_points);
      std::cout<<"warp_overlay cols:"<<warped_overlay.cols<<"warp_overlay rows:"<<warped_overlay.rows<<std::endl;
      std::cout<<"image height:"<<image->height<<"image width"<<image->width<<std::endl;
      cv::warpPerspective(overlay, warped_overlay, transform, cv::Size(warped_overlay.cols, warped_overlay.rows), 0, cv::BORDER_TRANSPARENT); 
    } 
    else {
      // TODO: Does not work
      cv::Mat transform = cv::Mat::eye(4, 4, CV_64F);
      CvMat ipltransform = transform;
      pose.GetMatrix(&ipltransform);
      //std::cout<<transform<<std::endl;
      cv::warpPerspective(overlay, warped_overlay, transform, cv::Size(warped_overlay.cols, warped_overlay.rows), 0, cv::BORDER_TRANSPARENT); 
    }

    cv::Mat frame(image);
    addFgWithAlpha(frame, warped_overlay);
  }
#endif
  //if (!overlay.data) {
  //  set_overlay("/tmp/overlay.png", "Test");
  //}
 #if 0 
  // Show overlay if we have any
  detectedMarkersPrev = detectedMarkers;
  std::map<int,int>::iterator iter;
  for (iter = detectedMarkers.begin(); iter != detectedMarkers.end(); iter++) {
    iter->second = 0; // Reset counters (but do not forget that this was seen previously?)
  }
  if (!overlay.data) {
    pthread_mutex_unlock(&mMutex);
    return 0;
  }
  for (size_t i=0; i<marker_detector.markers->size(); i++) {
    if (i >= 32) break;

    int marker_id = (*(marker_detector.markers))[i].GetId();
    detectedMarkers[marker_id]++;

    cv::Mat warped_overlay(image->height, image->width, CV_8UC4);
    warped_overlay.setTo(cv::Scalar(0,0,0,0));

    if (overlayScale > 0.5f) {
      cv::Point2f source_points[4];
      cv::Point2f dest_points[4];
      float overlay_maxdim = std::max(overlay.rows, overlay.cols);
      overlay_maxdim /= overlayScale;
      source_points[0] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(-overlay_maxdim/2,overlay_maxdim/2);
      source_points[1] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(overlay_maxdim/2,overlay_maxdim/2);
      source_points[2] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(overlay_maxdim/2,-overlay_maxdim/2);
      source_points[3] = cv::Point2f(overlay.cols/2, overlay.rows/2) + cv::Point2f(-overlay_maxdim/2,-overlay_maxdim/2);
      //source_points[0] = cv::Point2f(0,overlay.rows-1);
      //source_points[1] = cv::Point2f(overlay.cols-1,overlay.rows-1);
      //source_points[2] = cv::Point2f(overlay.cols-1,0);
      //source_points[3] = cv::Point2f(0,0);
      dest_points[0] = cv::Point2f(
        (*(marker_detector.markers))[i].marker_corners_img[0].x+15,
        (*(marker_detector.markers))[i].marker_corners_img[0].y+15);
      dest_points[1] = cv::Point2f(
        (*(marker_detector.markers))[i].marker_corners_img[1].x+15,
        (*(marker_detector.markers))[i].marker_corners_img[1].y+15);
      dest_points[2] = cv::Point2f(
        (*(marker_detector.markers))[i].marker_corners_img[2].x+15,
        (*(marker_detector.markers))[i].marker_corners_img[2].y+15);
      dest_points[3] = cv::Point2f(
        (*(marker_detector.markers))[i].marker_corners_img[3].x+15,
        (*(marker_detector.markers))[i].marker_corners_img[3].y+15);

      cv::Mat transform = cv::getPerspectiveTransform(source_points, dest_points);
      std::cout<<"warped_overlay cols:"<<warped_overlay.cols<<"warped overlay rows:"<<warped_overlay.rows<<std::endl;
      std::cout<<"image height:"<<image->height<<"image width"<<image->width<<std::endl;
      cv::warpPerspective(overlay, warped_overlay, transform, cv::Size(warped_overlay.cols, warped_overlay.rows), 0, cv::BORDER_TRANSPARENT); 
    } else {
      // TODO: Does not work
      cv::Mat transform = cv::Mat::eye(4, 4, CV_64F);
      CvMat ipltransform = transform;
      (*(marker_detector.markers))[i].pose.GetMatrix(&ipltransform);
      //std::cout<<transform<<std::endl;
      //cv::warpPerspective(overlay, warped_overlay, transform, cv::Size(warped_overlay.cols, warped_overlay.rows), 0, cv::BORDER_TRANSPARENT); 
    }
    cv::Mat frame(image);
    addFgWithAlpha(frame, warped_overlay);
  }
#endif
  // Update missing markers to the list of detectedMarkers
/*
  for (size_t i=0; i<detectedMarkersThisFrame.size(); i++) {
    if (detectedMarkers.find(detectedMarkersThisFrame[i]) == detectedMarkers.end()) {
      detectedMarkers[detectedMarkersThisFrame[i]] = 0;
    }
  }
  // Update counters in the detectedMarkers (found++, not-found--)
  std::map<int, int>::iterator iter;
  for (iter=detectedMarkers.begin(); iter != detectedMarkers.end(); iter++) {
    if (std::find(detectedMarkersThisFrame.begin(), detectedMarkersThisFrame.end(), iter->first) == detectedMarkersThisFrame.end()) {
      if (iter->second >= 0) iter->second = -1;
      else iter->second--; // Count how many frames we have not seen this
      // TODO: Delete: after < -3 ???
    } else {
      if (iter->second < 0) iter->second = 1;
      else iter->second++;
    }
  }
*/
  pthread_mutex_unlock(&mMutex);
  return marker_detector.markers->size();
}
