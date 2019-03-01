#include "rsschannellist.h"
#include "rsschannel.h"
#include "bettergramservice.h"

#include <styles/style_chat_helpers.h>
#include <logs.h>

#include <QCryptographicHash>
#include <QJsonDocument>

namespace Bettergram {

const int RssChannelList::_defaultFreq = 60;

QString RssChannelList::getName(NewsType newsType)
{
	switch(newsType) {
	case(NewsType::News):
		return "news";
	case (NewsType::Videos):
		return "videos";
	default:
		LOG(("Unable to recognize news type '%1'").arg(static_cast<int>(newsType)));
		return QString();
	}
}

int RssChannelList::getImageWidth(NewsType newsType)
{
	switch(newsType) {
	case(NewsType::News):
		return st::newsPanImageWidth;
	case (NewsType::Videos):
		return st::videosPanImageWidth;
	default:
		LOG(("Unable to recognize news type '%1'").arg(static_cast<int>(newsType)));
		return 0;
	}
}

int RssChannelList::getImageHeight(NewsType newsType)
{
	switch(newsType) {
	case(NewsType::News):
		return st::newsPanImageHeight;
	case (NewsType::Videos):
		return st::videosPanImageHeight;
	default:
		LOG(("Unable to recognize news type '%1'").arg(static_cast<int>(newsType)));
		return 0;
	}
}

RssChannelList::RssChannelList(NewsType newsType, QObject *parent) :
	QObject(parent),
	_newsType(newsType),
	_name(getName(newsType)),
	_imageWidth(getImageWidth(newsType)),
	_imageHeight(getImageHeight(newsType)),
	_freq(_defaultFreq),
	_lastUpdateString(BettergramService::defaultLastUpdateString())
{
}

RssChannelList::NewsType RssChannelList::newsType() const
{
	return _newsType;
}

int RssChannelList::freq() const
{
	return _freq;
}

void RssChannelList::setFreq(int freq)
{
	if (freq <= 0) {
		freq = _defaultFreq;
	}

	if (_freq != freq) {
		_freq = freq;
		emit freqChanged();
	}
}

QDateTime RssChannelList::lastUpdate() const
{
	return _lastUpdate;
}

QString RssChannelList::lastUpdateString() const
{
	return _lastUpdateString;
}

void RssChannelList::setLastUpdate(const QDateTime &lastUpdate)
{
	if (_lastUpdate != lastUpdate) {
		_lastUpdate = lastUpdate;

		_lastUpdateString = BettergramService::generateLastUpdateString(_lastUpdate, true);
		emit lastUpdateChanged();
	}
}

RssChannelList::const_iterator RssChannelList::begin() const
{
	return _list.begin();
}

RssChannelList::const_iterator RssChannelList::end() const
{
	return _list.end();
}

const QSharedPointer<RssChannel> &RssChannelList::at(int index) const
{
	if (index < 0 || index >= _list.size()) {
		LOG(("Index is out of bounds"));
		throw std::out_of_range("rss channel index is out of range");
	}

	return _list.at(index);
}

bool RssChannelList::isEmpty() const
{
	return _list.isEmpty();
}

int RssChannelList::count() const
{
	return _list.count();
}

int RssChannelList::countAllItems() const
{
	int result = 0;

	for (const QSharedPointer<RssChannel> &channel : _list) {
		result += channel->count();
	}

	return result;
}

int RssChannelList::countAllUnreadItems() const
{
	int result = 0;

	for (const QSharedPointer<RssChannel> &channel : _list) {
		result += channel->countUnread();
	}

	return result;
}

void RssChannelList::add(const QUrl &channelLink)
{
	if (channelLink.isEmpty()) {
		LOG(("Unable to add empty RSS channel"));
		return;
	}

	if (contains(channelLink)) {
		return;
	}

	QSharedPointer<RssChannel> channel(new RssChannel(channelLink, _imageWidth, _imageHeight));
	add(channel);
}

bool RssChannelList::contains(const QUrl &channelLink)
{
	for (const QSharedPointer<RssChannel> &channel : _list) {
		if (channel->feedLink() == channelLink) {
			return true;
		}
	}

	return false;
}

void RssChannelList::add(QSharedPointer<RssChannel> &channel)
{
	connect(channel.data(), &RssChannel::iconChanged, this, &RssChannelList::iconChanged);
	connect(channel.data(), &RssChannel::isReadChanged, this, &RssChannelList::onIsReadChanged);

	_list.push_back(channel);
}

void RssChannelList::markAsRead()
{
	for (QSharedPointer<RssChannel> &channel : _list) {
		disconnect(channel.data(), &RssChannel::isReadChanged, this, &RssChannelList::onIsReadChanged);

		channel->markAsRead();

		connect(channel.data(), &RssChannel::isReadChanged, this, &RssChannelList::onIsReadChanged);
	}

	onIsReadChanged();
}

QList<QSharedPointer<RssItem>> RssChannelList::getAllItems() const
{
	QList<QSharedPointer<RssItem>> result;
	result.reserve(countAllItems());

	for (const QSharedPointer<RssChannel> &channel : _list) {
		result.append(channel->getAllItems());
	}

	return result;
}

QList<QSharedPointer<RssItem>> RssChannelList::getAllUnreadItems() const
{
	QList<QSharedPointer<RssItem>> result;
	result.reserve(countAllUnreadItems());

	for (const QSharedPointer<RssChannel> &channel : _list) {
		result.append(channel->getAllUnreadItems());
	}

	return result;
}

void RssChannelList::parseFeeds()
{
	for (const QSharedPointer<RssChannel> &channel : _list) {
		if (channel->isFetching()) {
			return;
		}
	}

	bool isChanged = false;
	bool isAtLeastOneUpdated = false;

	for (const QSharedPointer<RssChannel> &channel : _list) {
		if (!channel->isFailed()) {
			isAtLeastOneUpdated = true;

			if (channel->parse()) {
				isChanged = true;
			}
		}
	}

	if (isChanged) {
		save();
		emit updated();
	}

	if (isAtLeastOneUpdated) {
		setLastUpdate(QDateTime::currentDateTime());
	}
}

void RssChannelList::parseChannelList(const QByteArray &channelList)
{
	// Update only if it has been changed
	QByteArray hash = QCryptographicHash::hash(channelList, QCryptographicHash::Sha256);

	if (hash == _lastSourceHash) {
		return;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(channelList, &parseError);

	if (!doc.isObject()) {
		LOG(("Can not get rss channel list. Data is wrong. %1 (%2). Data: %3")
			.arg(parseError.errorString())
			.arg(parseError.error)
			.arg(QString::fromUtf8(channelList)));

		return;
	}

	QJsonObject json = doc.object();

	if (json.isEmpty()) {
		LOG(("Can not get rss channel list. Data is emtpy or wrong"));
		return;
	}

	parseChannelList(json);
	_lastSourceHash = hash;
}

void RssChannelList::parseChannelList(const QJsonObject &json)
{
	if (json.isEmpty()) {
		return;
	}

	if (!json.value("success").toBool()) {
		return;
	}

	_list.clear();

	QJsonArray channelsJson = json.value(_name).toArray();

	for (const QJsonValue value : channelsJson) {
		if (!value.isString()) {
			LOG(("Unable to get channel address for %1").arg(_name));
			continue;
		}

		add(value.toString());
	}

	emit updated();
}

void RssChannelList::save()
{
	QSettings settings(BettergramService::instance()->settingsPath(_name), QSettings::IniFormat);

	settings.beginGroup(_name);

	settings.setValue("lastUpdate", _lastUpdate);
	settings.setValue("frequency", _freq);
	settings.beginWriteArray("channels", _list.size());

	for (int i = 0; i < _list.size(); i++) {
		const QSharedPointer<RssChannel> &channel = _list.at(i);

		settings.setArrayIndex(i);
		channel->save(settings);
	}

	settings.endArray();
	settings.endGroup();
}

void RssChannelList::load()
{
	QSettings settings(BettergramService::instance()->settingsPath(_name), QSettings::IniFormat);

	settings.beginGroup(_name);

	setLastUpdate(settings.value("lastUpdate").toDateTime());
	setFreq(settings.value("frequency", _defaultFreq).toInt());

	int size = settings.beginReadArray("channels");

	for (int i = 0; i < size; i++) {
		QSharedPointer<RssChannel> channel(new RssChannel(_imageWidth, _imageHeight));

		settings.setArrayIndex(i);
		channel->load(settings);

		add(channel);
	}

	settings.endArray();
	settings.endGroup();
}

void RssChannelList::onIsReadChanged()
{
	save();
}

} // namespace Bettergrams
