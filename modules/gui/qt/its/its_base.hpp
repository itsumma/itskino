#ifndef VLC_QT_ITS_BASE_HPP_
#define VLC_QT_ITS_BASE_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <memory>
#include <functional>

#include <QObject>
#include <QEvent>

namespace its {

// Callback type alias
template <typename Signature>
using Fn = std::function<Signature>;

} // its

#endif // include-guard

