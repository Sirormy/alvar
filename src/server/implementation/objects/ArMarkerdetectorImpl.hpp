/* Autogenerated with kurento-module-creator */

#ifndef __AR_MARKERDETECTOR_IMPL_HPP__
#define __AR_MARKERDETECTOR_IMPL_HPP__

#include "OpenCVFilterImpl.hpp"
#include "ArMarkerdetector.hpp"
#include <EventHandler.hpp>
#include <boost/property_tree/ptree.hpp>
#include "ArMarkerdetectorOpenCVImpl.hpp"

namespace kurento
{
namespace module
{
namespace markerdetector
{
class ArMarkerdetectorImpl;
} /* markerdetector */
} /* module */
} /* kurento */

namespace kurento
{
void Serialize (std::shared_ptr<kurento::module::markerdetector::ArMarkerdetectorImpl> &object, JsonSerializer &serializer);
} /* kurento */

namespace kurento
{
class MediaPipelineImpl;
} /* kurento */

namespace kurento
{
namespace module
{
namespace markerdetector
{

class ArMarkerdetectorImpl : public OpenCVFilterImpl, public virtual ArMarkerdetector, public virtual ArMarkerdetectorOpenCVImpl
{

public:

  ArMarkerdetectorImpl (const boost::property_tree::ptree &config, std::shared_ptr<MediaPipeline> mediaPipeline);

  virtual ~ArMarkerdetectorImpl () {};

  void setShowDebugLevel (int showDebugLevel);
  void setOffset (double x, double y, double z);
  int getShowDebugLevel ();
  void setOverlayImage (const std::string &overlayImage);
  std::string getOverlayImage ();
  void setOverlayText (const std::string &overlayText);
  std::string getOverlayText ();
  void setOverlayScale (float overlayScale);
  float getOverlayScale ();

  // TODO: Kludge, because ArMarkerdetectorOpenCVImpl is not inherited from MediaObject (?)
  virtual std::shared_ptr<MediaObject> getSharedFromThis() { return shared_from_this(); }

  /* Next methods are automatically implemented by code generator */
  virtual bool connect (const std::string &eventType, std::shared_ptr<EventHandler> handler);
  virtual void invoke (std::shared_ptr<MediaObjectImpl> obj,
                       const std::string &methodName, const Json::Value &params,
                       Json::Value &response);

  virtual void Serialize (JsonSerializer &serializer);

private:

  class StaticConstructor
  {
  public:
    StaticConstructor();
  };

  static StaticConstructor staticConstructor;

};

} /* markerdetector */
} /* module */
} /* kurento */

#endif /*  __AR_MARKERDETECTOR_IMPL_HPP__ */
