
#include "Window.h"
#include <WindowConfig.h>
#include "Config.h"

#if QT_VERSION_MAJOR >= 6
#include <effect/effecthandler.h>
#else
#include <kwineffects.h>
#endif

namespace
{
bool isFirefoxDocumentPipCandidate(const KWin::EffectWindow *window)
{
    const auto windowClass = window->windowClass().simplified().toLower();
    const auto windowRole  = window->windowRole().trimmed().toLower();
    const auto geometry    = window->frameGeometry();

    constexpr qreal maxInitialSize = 900.0;

    return (windowClass == QStringLiteral("navigator firefox") ||
            windowClass == QStringLiteral("firefox firefox")) &&
           windowRole == QStringLiteral("browser") && window->isNormalWindow() && !window->isUtility() &&
           window->keepAbove() && geometry.width() > 0 && geometry.height() > 0 &&
           geometry.width() <= maxInitialSize && geometry.height() <= maxInitialSize;
}
} // namespace

ShapeCorners::Window::Window(KWin::EffectWindow *kwindow) :
    w(kwindow), lastAnimationDuration(Config::animationDuration()), currentConfig(WindowConfig::inactiveWindowConfig()),
    isFirefoxDocumentPip(isFirefoxDocumentPipCandidate(kwindow))
{
    connect(Config::self(), &Config::configChanged, this, &Window::configChanged);
    configChanged();
}

bool ShapeCorners::Window::isActive() const { return KWin::effects->activeWindow() == w; }

bool ShapeCorners::Window::hasEffect() const
{
    const auto caption = w->caption();
    const auto isGoCDPip = isFirefoxDocumentPip &&
                           (caption.endsWith(QStringLiteral(" — GoCD"), Qt::CaseInsensitive) ||
                            caption.endsWith(QStringLiteral(" — GoCD — Mozilla Firefox"),
                                             Qt::CaseInsensitive));

    return w->expandedGeometry().isValid() && isGoCDPip && !isExcluded &&
           (hasRoundCorners() || hasOutline());
}

bool ShapeCorners::Window::hasRoundCorners() const
{
    if (currentConfig.cornerRadius <= 0) {
        return false;
    }
    if (w->isFullScreen()) {
        return !Config::disableRoundFullScreen();
    }
    if (isMaximized) {
        return !Config::disableRoundMaximize();
    }
    if (isTiled) {
        return !Config::disableRoundTile();
    }
    return true;
}

bool ShapeCorners::Window::hasOutline() const
{
    if (w->isFullScreen()) {
        return !Config::disableOutlineFullScreen();
    }
    if (isMaximized) {
        return !Config::disableOutlineMaximize();
    }
    if (isTiled) {
        return !Config::disableOutlineTile();
    }
    return true;
}

#ifdef QT_DEBUG
QDebug KWin::operator<<(const QDebug &debug, const EffectWindow &kwindow)
{
    return (debug << kwindow.windowType() << kwindow.windowClass() << kwindow.caption());
}
#endif

void ShapeCorners::Window::configChanged()
{
    isExcluded = false;
    isIncluded = false;
    for (auto &exclusion: Config::exclusions()) {
        if (w->windowClass().contains(exclusion, Qt::CaseInsensitive) ||
            captionAfterDash().contains(exclusion, Qt::CaseInsensitive)) {
            isExcluded = true;
#ifdef DEBUG_INCLUSIONS
            qDebug() << "ShapeCorners: Excluded window:" << *this;
#endif
            return;
        }
    }
    for (auto &inclusion: Config::inclusions()) {
        if (w->windowClass().contains(inclusion, Qt::CaseInsensitive) ||
            captionAfterDash().contains(inclusion, Qt::CaseInsensitive)) {
            isIncluded = true;
#ifdef DEBUG_INCLUSIONS
            qDebug() << "ShapeCorners: Included window:" << *this;
#endif
            return;
        }
    }
}

QJsonObject ShapeCorners::Window::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("class")]   = w->windowClass();
    json[QStringLiteral("caption")] = w->caption();
    return json;
}

QString ShapeCorners::Window::captionAfterDash() const
{
    const auto sep   = QStringLiteral(" — ");
    const auto index = w->caption().indexOf(sep);
    if (index == -1) {
        return w->caption();
    }
    return w->caption().mid(index + sep.size());
}
