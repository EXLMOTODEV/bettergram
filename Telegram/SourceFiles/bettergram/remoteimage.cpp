#include "remoteimage.h"

namespace Bettergram {

RemoteImage::RemoteImage(QObject *parent) :
	AbstractRemoteFile(parent)
{
}

RemoteImage::RemoteImage(const QUrl &link, bool isNeedDownloadIcon, QObject *parent) :
	AbstractRemoteFile(link, isNeedDownloadIcon, parent)
{
}

RemoteImage::RemoteImage(const QUrl &link,
						 int scaledWidth,
						 int scaledHeight,
						 bool isNeedDownloadIcon,
						 QObject *parent) :
	AbstractRemoteFile(link, isNeedDownloadIcon, parent),
	_scaledWidth(scaledWidth),
	_scaledHeight(scaledHeight)
{
}

RemoteImage::RemoteImage(int scaledWidth, int scaledHeight, QObject *parent) :
	AbstractRemoteFile(parent),
	_scaledWidth(scaledWidth),
	_scaledHeight(scaledHeight)
{
}

int RemoteImage::scaledWidth() const
{
	return _scaledWidth;
}

void RemoteImage::setScaledWidth(int scaledWidth)
{
	if (_scaledWidth != scaledWidth) {
		_scaledWidth = scaledWidth;

		stopDownloadLaterTimer();
		download();
	}
}

int RemoteImage::scaledHeight() const
{
	return _scaledHeight;
}

void RemoteImage::setScaledHeight(int scaledHeight)
{
	if (_scaledHeight != scaledHeight) {
		_scaledHeight = scaledHeight;

		stopDownloadLaterTimer();
		download();
	}
}

void RemoteImage::setScaledSize(int scaledWidth, int scaledHeight)
{
	bool isChanged = false;

	if (_scaledWidth != scaledWidth) {
		_scaledWidth = scaledWidth;
		isChanged = true;
	}

	if (_scaledHeight != scaledHeight) {
		_scaledHeight = scaledHeight;
		isChanged = true;
	}

	if (isChanged) {
		stopDownloadLaterTimer();
		download();
	}
}

const QPixmap &RemoteImage::image() const
{
	return _image;
}

bool RemoteImage::isNull() const
{
	return _image.isNull();
}

bool RemoteImage::customIsNeedToDownload() const
{
	return isNull();
}

void RemoteImage::dataDownloaded(const QByteArray &data)
{
	if (data.isEmpty()) {
		resetData();
		return;
	}

	QPixmap image;

	if (!image.loadFromData(data)) {
		LOG(("Can not get image from %1. Can not convert response to image.")
			.arg(link().toString()));

		resetData();
		return;
	}

	setImage(image);
}

void RemoteImage::setImage(const QPixmap &image)
{
	if (!image.isNull() &&
			((_scaledWidth && image.width() > _scaledWidth)
			 || (_scaledHeight && image.height() > _scaledHeight))) {

		if (_scaledWidth && _scaledHeight) {
			_image = image.scaled(_scaledWidth,
								  _scaledHeight,
								  Qt::KeepAspectRatioByExpanding,
								  Qt::SmoothTransformation);
		} else if (_scaledWidth) {
			_image = image.scaledToWidth(_scaledWidth, Qt::SmoothTransformation);
		} else {
			_image = image.scaledToHeight(_scaledHeight, Qt::SmoothTransformation);
		}
	} else {
		_image = image;
	}

	emit imageChanged();
}

void RemoteImage::resetData()
{
	if (!_image.isNull()) {
		_image = QPixmap();

		emit imageChanged();
	}
}

bool RemoteImage::checkLink(const QUrl &link)
{
	// Do not download images with too large file names
	// because it seems they are auto generated not user visible images

	return link.toString().size() < 300;
}

} // namespace Bettergrams
