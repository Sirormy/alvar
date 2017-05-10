/* Autogenerated with kurento-module-creator */

#include "ArMarkerdetectorOpenCVImpl.hpp"
#include <KurentoException.hpp>

namespace kurento
{
namespace module
{
namespace markerdetector
{

ArMarkerdetectorOpenCVImpl::ArMarkerdetectorOpenCVImpl ()
{
}

/*
 * This function will be called with each new frame. mat variable
 * contains the current frame. You should insert your image processing code
 * here. Any changes in mat, will be sent through the Media Pipeline.
 */
void ArMarkerdetectorOpenCVImpl::process (cv::Mat &mat)
{
  IplImage ipl = mat;
  ar.detect_marker(&ipl);
  std::map<int,int>::iterator iter;
  for (iter = ar.detectedMarkers.begin(); iter != ar.detectedMarkers.end(); iter++) {
    int marker_id = iter->first;
    int marker_count = iter->second;
    int marker_count_prev = ar.detectedMarkersPrev[marker_id];
    if (marker_count == marker_count_prev) continue;
    try {
      MarkerCount event(getSharedFromThis(), MarkerCount::getName(), marker_id,
                        marker_count, marker_count-marker_count_prev);
      signalMarkerCount(event);
    } catch (std::bad_weak_ptr &e) {}
  }
}


void ArMarkerdetectorOpenCVImpl::setShowDebugLevel (int showDebugLevel)
{
    ar.setShowDebugLevel(showDebugLevel);
}

void ArMarkerdetectorOpenCVImpl::setOffset (double x, double y, double z)
{
	  ar.setOffset(x,y,z);
}

int ArMarkerdetectorOpenCVImpl::getShowDebugLevel ()
{
  return ar.getShowDebugLevel();
}

void ArMarkerdetectorOpenCVImpl::setOverlayImage (const std::string &overlayImage)
{
  ar.set_overlay(overlayImage.c_str(), NULL);
}

std::string ArMarkerdetectorOpenCVImpl::getOverlayImage ()
{
  return ar.get_overlay_image();
}

void ArMarkerdetectorOpenCVImpl::setOverlayText (const std::string &overlayText)
{
  ar.set_overlay(NULL, overlayText.c_str());
}

std::string ArMarkerdetectorOpenCVImpl::getOverlayText ()
{
  return ar.get_overlay_text();
}

void ArMarkerdetectorOpenCVImpl::setOverlayScale (float overlayScale)
{
  ar.set_overlay_scale(overlayScale);
}

float ArMarkerdetectorOpenCVImpl::getOverlayScale ()
{
  return ar.get_overlay_scale();
}

} /* markerdetector */
} /* module */
} /* kurento */
