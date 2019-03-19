#include "bettergramservice.h"
#include "cryptopricelist.h"
#include "cryptoprice.h"
#include "rsschannellist.h"
#include "rsschannel.h"
#include "resourcegrouplist.h"
#include "pinnednewslist.h"
#include "aditem.h"

#include <auth_session.h>
#include <mainwidget.h>
#include <messenger.h>
#include <settings.h>
#include <core/update_checker.h>
#include <core/click_handler_types.h>
#include <lang/lang_keys.h>
#include <platform/platform_specific.h>
#include <boxes/confirm_box.h>

#include <QCoreApplication>
#include <QTimer>
#include <QTimerEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Bettergram {

BettergramService *BettergramService::_instance = nullptr;
const QString BettergramService::_defaultLastUpdateString = "...";
const QString BettergramService::_pricesUrlPrefix = "http-api";

const int BettergramService::_networkTimeout = 10 * 1000;

// We check for new updates in 2 minutes after application startup
const int BettergramService::_checkForFirstUpdatesDelay = 2 * 60 * 1000;

// We check for new updates every 10 hours
const int BettergramService::_checkForUpdatesPeriod = 10 * 60 * 60 * 1000;

// We update crypto price names every day
const int BettergramService::_updateCryptoPriceNamesPeriod = 1 * 24 * 60 * 60 * 1000;

// We save crypto prices every 2 hours
const int BettergramService::_saveCryptoPricesPeriod = 2 * 60 * 60 * 1000;

// We update rss channel list every 2 hours
const int BettergramService::_updateRssChannelListPeriod = 2 * 60 * 60 * 1000;

// We update video channel list every 2 hours
const int BettergramService::_updateVideoChannelListPeriod = 2 * 60 * 60 * 1000;

// We display deprecated API messages no more than once per 2 hours
const int BettergramService::_deprecatedApiMessagePeriod = 2 * 60 * 60 * 1000;

BettergramService *BettergramService::init()
{
	return instance();
}

BettergramService *BettergramService::instance()
{
	if (!_instance) {
		new BettergramService(nullptr);
	}

	return _instance;
}

int BettergramService::networkTimeout()
{
	return _networkTimeout;
}

const QString &BettergramService::defaultLastUpdateString()
{
	return _defaultLastUpdateString;
}

QString BettergramService::generateLastUpdateString(const QDateTime &dateTime, bool isShowSeconds)
{
	if (dateTime.isNull()) {
		return _defaultLastUpdateString;
	}

	qint64 daysBefore = QDateTime::currentDateTime().daysTo(dateTime);

	const QString timeFormat = isShowSeconds ? "hh:mm:ss" : "hh:mm";
	const QString timeString = dateTime.toString(timeFormat);

	if (daysBefore == 0) {
		return lng_player_message_today(lt_time, timeString);
	} else if (daysBefore == -1) {
		return lng_player_message_yesterday(lt_time, timeString);
	} else {
		return lng_player_message_date(lt_date,
									   langDayOfMonthFull(dateTime.date()),
									   lt_time,
									   timeString);
	}
}

void BettergramService::openUrl(UrlSource urlSource, const QUrl &url)
{
	QString urlString = url.toString();
	UrlClickHandler::Open(urlString);
}

bool BettergramService::isBettergramTabsShowed()
{
	return App::main() && App::main()->isBettergramTabsShowed();
}

void BettergramService::toggleBettergramTabs()
{
	Q_ASSERT(_instance);

	emit _instance->needToToggleBettergramTabs();
}

Bettergram::BettergramService::BettergramService(QObject *parent) :
	QObject(parent),
	_cryptoPriceList(new CryptoPriceList(this)),
	_rssChannelList(new RssChannelList(RssChannelList::NewsType::News, this)),
	_videoChannelList(new RssChannelList(RssChannelList::NewsType::Videos, this)),
	_resourceGroupList(new ResourceGroupList(this)),
	_pinnedNewsList(new PinnedNewsList(this)),
	_currentAd(new AdItem(this))
{
	_instance = this;

	getIsPaid();
	getNextAd(true);

	_rssChannelList->load();

    if (_rssChannelList->isEmpty()) {
        _rssChannelList->add(QUrl("https://news.livecoinwatch.com/feed/"));
        _rssChannelList->add(QUrl("https://thetokenist.io/feed/"));
        _rssChannelList->add(QUrl("https://iamnye.com/blog/feed/"));
    }

	_videoChannelList->load();

    if (_videoChannelList->isEmpty()) {
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCXyrBCWaRJzHfOtnWaR47Qw"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCbkjUYiPN8P48r0lurEBP8w"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCu1-oBOM-DzJ89o02Bx3XYw"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCiUnrCUGCJTCC7KjuW493Ww"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCu7Sre5A1NMV8J3s2FhluCw"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCCatR7nWbYrkVXdxXb4cGXw"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCc4Rz_T9Sb1w5rqqo9pL1Og"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UC4sS8q8E5ayyghbhiPon4uw"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCNfIaEvbasoC_yIGz5xnE4g"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UC67AEEecqFEc92nVvcqKdhA"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?channel_id=UCspcykxjIbHo0C9ZOCM9YnA"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?user=obham001"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?user=yourmom7829"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?user=Diaryofamademan"));
        _videoChannelList->add(QUrl("https://www.youtube.com/feeds/videos.xml?user=LiljeqvistIvan"));
	}

	connect(_rssChannelList, &RssChannelList::update,
			this, &BettergramService::onUpdateRssFeedsContent);

	connect(_videoChannelList, &RssChannelList::update,
			this, &BettergramService::onUpdateVideoFeedsContent);

	getRssChannelList();
	getVideoChannelList();

	getRssFeedsContent();
	getVideoFeedsContent();

	if (QFile::exists(resourcesCachePath())) {
		if (!_resourceGroupList->parseFile(resourcesCachePath())) {
			_resourceGroupList->parseFile(":/bettergram/default-resources.json");
		}
	} else {
		_resourceGroupList->parseFile(":/bettergram/default-resources.json");
	}

	getResourceGroupList();
	getPinnedNewsList();

	_cryptoPriceList->load();

	getCryptoPriceNames();

	_updateCryptoPriceNamesTimerId = startTimer(_updateCryptoPriceNamesPeriod, Qt::VeryCoarseTimer);
	_saveCryptoPricesTimerId = startTimer(_saveCryptoPricesPeriod, Qt::VeryCoarseTimer);

	_updateRssChannelListTimerId = startTimer(_updateRssChannelListPeriod, Qt::VeryCoarseTimer);
	_updateVideoChannelListTimerId = startTimer(_updateVideoChannelListPeriod, Qt::VeryCoarseTimer);

	_everyDayTimerId = startTimer(24 * 60 * 60 * 1000, Qt::VeryCoarseTimer);

	connect(qApp, &QCoreApplication::aboutToQuit, this, [this] { _cryptoPriceList->save(); });

	QTimer::singleShot(_checkForFirstUpdatesDelay, Qt::VeryCoarseTimer,
					   this, [] { checkForNewUpdates(); });

	_checkForUpdatesTimerId = startTimer(_checkForUpdatesPeriod, Qt::VeryCoarseTimer);

	Platform::RegisterCustomScheme();

	_isSettingsPorted = QSettings(bettergramSettingsPath(), QSettings::IniFormat).value("isSettingsPorted").toBool();
}

void BettergramService::portSettingsFiles()
{
	if (_isSettingsPorted) {
		return;
	}

	QSettings oldSettings;
	QSettings newSettings(bettergramSettingsPath(), QSettings::IniFormat);

	oldSettings.beginGroup(Auth().user()->phone());
	newSettings.beginGroup(Auth().user()->phone());

	oldSettings.beginGroup("favorites");
	newSettings.beginGroup("favorites");

	portSettingsFiles(oldSettings, newSettings);

	oldSettings.endGroup();
	newSettings.endGroup();

	oldSettings.beginGroup("pinned");
	newSettings.beginGroup("pinned");

	portSettingsFiles(oldSettings, newSettings);

	oldSettings.endGroup();
	newSettings.endGroup();

	newSettings.endGroup();

	newSettings.sync();

	oldSettings.remove("favorites");
	oldSettings.remove("pinned");

	oldSettings.endGroup();

	oldSettings.remove("last_tab");
	oldSettings.sync();

	newSettings.setValue("isSettingsPorted", true);
	newSettings.sync();

	_isSettingsPorted = true;
}

void BettergramService::portSettingsFiles(QSettings &oldSettings, QSettings &newSettings)
{
	for (const QString &key : oldSettings.allKeys()) {
		newSettings.setValue(key, oldSettings.value(key));
	}
}

bool BettergramService::isPaid() const
{
	return _isPaid;
}

void BettergramService::setIsPaid(bool isPaid)
{
	if (_isPaid != isPaid) {
		_isPaid = isPaid;

		emit isPaidChanged();
		_isPaidObservable.notify();
	}
}

BettergramService::BillingPlan BettergramService::billingPlan() const
{
	return _billingPlan;
}

void BettergramService::setBillingPlan(BillingPlan billingPlan)
{
	if (_billingPlan != billingPlan) {
		_billingPlan = billingPlan;

		emit billingPlanChanged();
		_billingPlanObservable.notify();
	}
}

CryptoPriceList *BettergramService::cryptoPriceList() const
{
	return _cryptoPriceList;
}

RssChannelList *BettergramService::rssChannelList() const
{
	return _rssChannelList;
}

RssChannelList *BettergramService::videoChannelList() const
{
	return _videoChannelList;
}

ResourceGroupList *BettergramService::resourceGroupList() const
{
	return _resourceGroupList;
}

PinnedNewsList *BettergramService::pinnedNewsList() const
{
	return _pinnedNewsList;
}

AdItem *BettergramService::currentAd() const
{
	return _currentAd;
}

bool BettergramService::isWindowActive() const
{
	return _isWindowActive;
}

void BettergramService::setIsWindowActive(bool isWindowActive)
{
	if (_isWindowActive != isWindowActive) {
		_isWindowActive = isWindowActive;

		if (_isWindowActiveHandler) {
			_isWindowActiveHandler();
		}
	}
}

base::Observable<void> &BettergramService::isPaidObservable()
{
	return _isPaidObservable;
}

base::Observable<void> &BettergramService::billingPlanObservable()
{
	return _billingPlanObservable;
}

QString BettergramService::settingsDirPath() const
{
	return cWorkingDir() + QStringLiteral("tdata/bettergram/");
}

QString BettergramService::cacheDirPath() const
{
	return settingsDirPath() + QStringLiteral("cache/");
}

QString BettergramService::pricesCacheDirPath() const
{
	return cacheDirPath() + QStringLiteral("prices/");
}

QString BettergramService::pricesIconsCacheDirPath() const
{
	return pricesCacheDirPath() + QStringLiteral("icons/");
}

QString BettergramService::resourcesCachePath() const
{
	return cacheDirPath() + QStringLiteral("resources.json");
}

QString BettergramService::settingsPath(const QString &name) const
{
	return settingsDirPath() + name + QStringLiteral(".ini");
}

QString BettergramService::bettergramSettingsPath() const
{
	return settingsPath(QStringLiteral("bettergram"));
}

QString BettergramService::pricesSettingsPath() const
{
	return settingsPath(QStringLiteral("prices"));
}

QString BettergramService::pricesCacheSettingsPath() const
{
	return pricesCacheDirPath() + QStringLiteral("prices.ini");
}

void BettergramService::getIsPaid()
{
	//TODO: bettergram: ask server and get know if the instance is paid or not and the current billing plan.
	//					If the application is not paid then call getNextAd();
}

void BettergramService::getCryptoPriceNames()
{
	QUrl url(QStringLiteral("https://%1.livecoinwatch.com/currencies").arg(_pricesUrlPrefix));

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished,
			this, &BettergramService::onGetCryptoPriceNamesFinished);

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get crypto price names due timeout"));
	});

	connect(reply, &QNetworkReply::sslErrors,
			this, &BettergramService::onSslFailed);
}

QUrl BettergramService::getCryptoPriceValues(int offset, int count)
{
	if (offset < 0 || offset >= _cryptoPriceList->count() || count <= 0) {
		QTimer::singleShot(0, _cryptoPriceList, [this] { _cryptoPriceList->emptyValues(); });
		return QUrl();
	}

	QUrl url(QStringLiteral("https://%1.livecoinwatch.com/coins?sort=%2&order=%3&offset=%4&limit=%5")
			 .arg(_pricesUrlPrefix)
			 .arg(_cryptoPriceList->sortString())
			 .arg(_cryptoPriceList->orderString())
			 .arg(offset)
			 .arg(count));

	getCryptoPriceValues(url);

	return url;
}

QUrl BettergramService::getCryptoPriceValues(int offset, int count, const QStringList &shortNames)
{
	if (offset < 0 || offset >= _cryptoPriceList->count() || count <= 0 || shortNames.isEmpty()) {
		QTimer::singleShot(0, _cryptoPriceList, [this] { _cryptoPriceList->emptyValues(); });
		return QUrl();
	}

	QUrl url(QStringLiteral("https://%1.livecoinwatch.com/coins?sort=%2&order=%3&offset=%4&limit=%5&only=%6")
			 .arg(_pricesUrlPrefix)
			 .arg(_cryptoPriceList->sortString())
			 .arg(_cryptoPriceList->orderString())
			 .arg(offset)
			 .arg(count)
			 .arg(shortNames.join(QStringLiteral(","))));

	getCryptoPriceValues(url);

	return url;
}

QUrl BettergramService::getSearchCryptoPriceValues(int offset, int count)
{
	if (offset < 0 || offset >= _cryptoPriceList->count() || count <= 0 || !_cryptoPriceList->isSearching()) {
		QTimer::singleShot(0, _cryptoPriceList, [this] { _cryptoPriceList->emptyValues(); });
		return QUrl();
	}

	QUrl url;

	if (_cryptoPriceList->sortOrder() == CryptoPriceList::SortOrder::Rank) {
		QStringList shortNames = _cryptoPriceList->getSearchListShortNames(offset, count);

		url = QStringLiteral("https://%1.livecoinwatch.com/coins?sort=none&limit=%2&only=%3")
				.arg(_pricesUrlPrefix)
				.arg(shortNames.size())
				.arg(shortNames.join(QStringLiteral(",")));
	} else {
		QStringList shortNames = _cryptoPriceList->getSearchListShortNames();

		url = QStringLiteral("https://%1.livecoinwatch.com/coins?sort=%2&order=%3&offset=%4&limit=%5&only=%6")
				.arg(_pricesUrlPrefix)
				.arg(_cryptoPriceList->sortString())
				.arg(_cryptoPriceList->orderString())
				.arg(offset)
				.arg(shortNames.size())
				.arg(shortNames.join(QStringLiteral(",")));
	}

	getCryptoPriceValues(url);

	return url;
}

void BettergramService::searchCryptoPriceNames()
{
	if (!_cryptoPriceList->isSearching()) {
		return;
	}

	const QString searchText = _cryptoPriceList->searchText();

	const QUrl url(QStringLiteral("https://%1.livecoinwatch.com/currencies?search=%2&type=coin")
				   .arg(_pricesUrlPrefix)
				   .arg(searchText));

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [this, url, searchText, reply] {
		if (isApiDeprecated(reply)) {
			return;
		}

		if(reply->error() == QNetworkReply::NoError) {
			// We parse the response only if the search text is the same
			if (_cryptoPriceList->searchText() == searchText) {
				_cryptoPriceList->parseSearchNames(reply->readAll());
			}
		} else {
			LOG(("Can not search crypto price values. Search text: '%1'. %2 (%3)")
				.arg(searchText)
				.arg(reply->errorString())
				.arg(reply->error()));
		}
	});

	connect(reply, &QNetworkReply::finished, [networkManager, reply, searchText]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply, searchText] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not search crypto price values due timeout. Search text: '%1'").arg(searchText));
	});

	connect(reply, &QNetworkReply::sslErrors, this, &BettergramService::onSslFailed);
}

void BettergramService::getCryptoPriceValues(const QUrl &url)
{
	if (!_cryptoPriceList->areNamesFetched()) {
		getCryptoPriceNames();
		return;
	}

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [this, url, reply] {
		if (isApiDeprecated(reply)) {
			return;
		}

		if(reply->error() == QNetworkReply::NoError) {
			_cryptoPriceList->parseValues(reply->readAll(), url);

			if (_cryptoPriceList->mayFetchStats()) {
				getCryptoPriceStats();
			}
		} else {
			LOG(("Can not get crypto price values. %1 (%2)")
				.arg(reply->errorString())
				.arg(reply->error()));
		}
	});

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get crypto price values due timeout"));
	});

	connect(reply, &QNetworkReply::sslErrors,
			this, &BettergramService::onSslFailed);
}

void BettergramService::getCryptoPriceStats()
{
	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(QStringLiteral("https://%1.livecoinwatch.com/stats").arg(_pricesUrlPrefix));

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [this, reply] {
		if (isApiDeprecated(reply)) {
			return;
		}

		if(reply->error() == QNetworkReply::NoError) {
			_cryptoPriceList->parseStats(reply->readAll());
		} else {
			LOG(("Can not get crypto price stats. %1 (%2)")
				.arg(reply->errorString())
				.arg(reply->error()));
		}
	});

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get crypto price stats due timeout"));
	});

	connect(reply, &QNetworkReply::sslErrors,
			this, &BettergramService::onSslFailed);
}

void BettergramService::getRssFeedsContent()
{
	for (const QSharedPointer<RssChannel> &channel : *_rssChannelList) {
		if (channel->isMayFetchNewData()) {
			getRssFeeds(_rssChannelList, channel);
		}
	}
}

void BettergramService::getVideoFeedsContent()
{
	for (const QSharedPointer<RssChannel> &channel : *_videoChannelList) {
		if (channel->isMayFetchNewData()) {
			getRssFeeds(_videoChannelList, channel);
		}
	}
}

void BettergramService::getRssFeeds(RssChannelList *rssChannelList,
									const QSharedPointer<RssChannel> &channel)
{
	channel->startFetching();

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(channel->feedLink());

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished, this, [rssChannelList, reply, channel] {
		if(reply->error() == QNetworkReply::NoError) {
			channel->fetchingSucceed(reply->readAll());
		} else {
			LOG(("Can not get RSS feeds from the channel %1. %2 (%3)")
				.arg(channel->feedLink().toString())
				.arg(reply->errorString())
				.arg(reply->error()));

			channel->fetchingFailed();
		}

		rssChannelList->parseFeeds();
	});

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply, channel] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get RSS feeds from the channel %1 due timeout")
			.arg(channel->feedLink().toString()));

		channel->fetchingFailed();
	});

	connect(reply, &QNetworkReply::sslErrors, this, [channel] (QList<QSslError> errors) {
		LOG(("Got SSL errors in during getting RSS feeds from the channel: %1")
			.arg(channel->feedLink().toString()));

		for(const QSslError &error : errors) {
			LOG(("%1").arg(error.errorString()));
		}
	});
}

void BettergramService::getRssChannelList()
{
	QUrl url("https://api.bettergram.io/v1/news");

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished,
			this, &BettergramService::onGetRssChannelListFinished);

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get RSS channel list due timeout"));
	});

	connect(reply, &QNetworkReply::sslErrors, this, &BettergramService::onSslFailed);
}

void BettergramService::getVideoChannelList()
{
	QUrl url("https://api.bettergram.io/v1/videos");

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished,
			this, &BettergramService::onGetVideoChannelListFinished);

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get video channel list due timeout"));
	});

	connect(reply, &QNetworkReply::sslErrors, this, &BettergramService::onSslFailed);
}

void BettergramService::getResourceGroupList()
{
	QUrl url("https://api.bettergram.io/v1/resources");

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished,
			this, &BettergramService::onGetResourceGroupListFinished);

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get resource group list due timeout"));
	});

	connect(reply, &QNetworkReply::sslErrors, this, &BettergramService::onSslFailed);
}

void BettergramService::getPinnedNewsList()
{
	QUrl url("https://api.bettergram.io/v1/pinned_news");

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished,
			this, &BettergramService::onGetPinnedNewsListFinished);

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();

		LOG(("Can not get pinned news list due timeout"));
	});

	connect(reply, &QNetworkReply::sslErrors, this, &BettergramService::onSslFailed);
}

void BettergramService::onGetCryptoPriceNamesFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (isApiDeprecated(reply)) {
		return;
	}

	if(reply->error() == QNetworkReply::NoError) {
		_cryptoPriceList->parseNames(reply->readAll());
	} else {
		LOG(("Can not get crypto price names. %1 (%2)")
			.arg(reply->errorString())
			.arg(reply->error()));
	}
}

void BettergramService::onSslFailed(QList<QSslError> errors)
{
	for(const QSslError &error : errors) {
		LOG(("%1").arg(error.errorString()));
	}
}

void BettergramService::onGetResourceGroupListFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (isApiDeprecated(reply)) {
		return;
	}

	if(reply->error() == QNetworkReply::NoError) {
		_resourceGroupList->parse(reply->readAll());
	} else {
		LOG(("Can not get resource group list. %1 (%2)")
			.arg(reply->errorString())
			.arg(reply->error()));
	}
}

void BettergramService::onGetPinnedNewsListFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (isApiDeprecated(reply)) {
		return;
	}

	if(reply->error() == QNetworkReply::NoError) {
		_pinnedNewsList->parse(reply->readAll());
	} else {
		LOG(("Can not get pinned news list. %1 (%2)")
			.arg(reply->errorString())
			.arg(reply->error()));
	}
}

void BettergramService::onGetRssChannelListFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (isApiDeprecated(reply)) {
		return;
	}

	if(reply->error() == QNetworkReply::NoError) {
		_rssChannelList->parseChannelList(reply->readAll());
		getRssFeedsContent();
	} else {
		LOG(("Can not get rss channel list. %1 (%2)")
			.arg(reply->errorString())
			.arg(reply->error()));
	}
}

void BettergramService::onGetVideoChannelListFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (isApiDeprecated(reply)) {
		return;
	}

	if(reply->error() == QNetworkReply::NoError) {
		_videoChannelList->parseChannelList(reply->readAll());
		getVideoFeedsContent();
	} else {
		LOG(("Can not get video channel list. %1 (%2)")
			.arg(reply->errorString())
			.arg(reply->error()));
	}
}

void BettergramService::getNextAd(bool reset)
{
	if(_isPaid) {
		_currentAd->clear();
		return;
	}

	QString url = "https://api.bettergram.io/v1/ads/next";

	if (!reset && !_currentAd->isEmpty()) {
		url += "?last=";
		url += _currentAd->id();
	}

	QNetworkAccessManager *networkManager = new QNetworkAccessManager();

	QNetworkRequest request;
	request.setUrl(url);

	QNetworkReply *reply = networkManager->get(request);

	connect(reply, &QNetworkReply::finished,
			this, &BettergramService::onGetNextAdFinished);

	connect(reply, &QNetworkReply::finished, [networkManager, reply]() {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	connect(this, &BettergramService::destroyed, networkManager, [networkManager, reply] {
		reply->deleteLater();
		networkManager->deleteLater();
	});

	QTimer::singleShot(_networkTimeout, Qt::VeryCoarseTimer, networkManager,
					   [networkManager, reply, this] {
		reply->deleteLater();
		networkManager->deleteLater();

		getNextAdLater();
	});

	connect(reply, &QNetworkReply::sslErrors,
			this, &BettergramService::onSslFailed);
}

void BettergramService::getNextAdLater(bool reset)
{
	int delay = _currentAd->duration();

	if (delay <= 0) {
		delay = AdItem::defaultDuration();
	}

	QTimer::singleShot(delay * 1000, this, [this, reset]() {
		if (_isWindowActive) {
			_isWindowActiveHandler = nullptr;
			getNextAd(reset);
		} else {
			_isWindowActiveHandler = [this, reset]() {
				if (_isWindowActive) {
					_isWindowActiveHandler = nullptr;
					getNextAd(reset);
				}
			};
		}
	});
}

bool BettergramService::parseNextAd(const QByteArray &byteArray)
{
	if (byteArray.isEmpty()) {
		LOG(("Can not get next ad. Response is emtpy"));
		return false;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

	if (!doc.isObject()) {
		LOG(("Can not get next ad. Response is wrong. %1 (%2). Response: %3")
			.arg(parseError.errorString())
			.arg(parseError.error)
			.arg(QString::fromUtf8(byteArray)));
		return false;
	}

	QJsonObject json = doc.object();

	if (json.isEmpty()) {
		LOG(("Can not get next ad. Response is emtpy or wrong"));
		return false;
	}

	bool isSuccess = json.value("success").toBool();

	if (!isSuccess) {
		QString errorMessage = json.value("message").toString("Unknown error");
		LOG(("Can not get next ad. %1").arg(errorMessage));
		return false;
	}

	QJsonObject adJson = json.value("ad").toObject();

	if (adJson.isEmpty()) {
		LOG(("Can not get next ad. Ad json is empty"));
		return false;
	}

	QString id = adJson.value("_id").toString();
	if (id.isEmpty()) {
		LOG(("Can not get next ad. Id is empty"));
		return false;
	}

	QString text = adJson.value("text").toString();
	if (text.isEmpty()) {
		LOG(("Can not get next ad. Text is empty"));
		return false;
	}

	QString url = adJson.value("url").toString();
	if (url.isEmpty()) {
		LOG(("Can not get next ad. Url is empty"));
		return false;
	}

	int duration = adJson.value("duration").toInt(AdItem::defaultDuration());

	AdItem adItem(id, text, url, duration, nullptr);

	_currentAd->update(adItem);

	return true;
}

void BettergramService::onGetNextAdFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (isApiDeprecated(reply)) {
		return;
	}

	if(reply->error() == QNetworkReply::NoError) {
		if (parseNextAd(reply->readAll())) {
			getNextAdLater();
		} else {
			// Try to get new ad without previous ad id
			getNextAdLater(true);
		}
	} else {
		//	LOG(("Can not get next ad item. %1 (%2)")
		//				  .arg(reply->errorString())
		//				  .arg(reply->error()));

		getNextAdLater();
	}
}

void BettergramService::onUpdateRssFeedsContent()
{
	getRssFeedsContent();
}

void BettergramService::onUpdateVideoFeedsContent()
{
	getVideoFeedsContent();
}

void BettergramService::checkForNewUpdates()
{
	LOG(("Check for new updates"));

	// We got this code from UpdateStateRow::onCheck() slot

	if (!cAutoUpdate()) {
		return;
	}

	Core::UpdateChecker checker;

	cSetLastUpdateCheck(0);
	checker.start();
}

bool BettergramService::isApiDeprecated(const QNetworkReply *reply)
{
	if (!reply) {
		LOG(("Unable to check for deprecated API because reply is null"));

		// The reply does not contain 410 (Gone) HTTP status,
		// but we can not continue without valid reply object
		return true;
	}

	QVariant statusCodeVariant = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

	if (!statusCodeVariant.isValid()) {
		if (reply->error() == QNetworkReply::ContentGoneError) {
			showDeprecatedApiMessage();
			return true;
		}

		LOG(("Unable to get HTTP status code. Url: %1").arg(reply->url().toString()));
		return false;
	}

	bool okInt = false;
	int statusCode = statusCodeVariant.toInt(&okInt);

	if (!okInt) {
		return false;
	}

	if (statusCode == 410) {
		showDeprecatedApiMessage();
		return true;
	} else {
		return false;
	}
}

void BettergramService::showDeprecatedApiMessage()
{
	const QDateTime now = QDateTime::currentDateTime();

	if (_isDeprecatedApiMessageShown
			|| (_lastTimeOfShowingDeprecatedApiMessage.isValid()
			&& (qAbs(_lastTimeOfShowingDeprecatedApiMessage.msecsTo(now)) < _deprecatedApiMessagePeriod))) {
		return;
	}

	_lastTimeOfShowingDeprecatedApiMessage = now;
	_isDeprecatedApiMessageShown = true;

	Ui::show(Box<InformBox>(lang(lng_bettergram_api_deprecated),
							[this] {
		_lastTimeOfShowingDeprecatedApiMessage = QDateTime::currentDateTime();
		_isDeprecatedApiMessageShown = false;
	}));
}

void BettergramService::everyDayActions()
{
	// We should update these data every day and it is not matter we show related tabs or not

	getPinnedNewsList();
	getResourceGroupList();
}

void BettergramService::timerEvent(QTimerEvent *timerEvent)
{
	if (timerEvent->timerId() == _checkForUpdatesTimerId) {
		checkForNewUpdates();
	} else if (timerEvent->timerId() == _updateCryptoPriceNamesTimerId) {
		getCryptoPriceNames();
	} else if (timerEvent->timerId() == _saveCryptoPricesTimerId) {
		_cryptoPriceList->save();
	} else if (timerEvent->timerId() == _updateRssChannelListTimerId) {
		getRssChannelList();
	} else if (timerEvent->timerId() == _updateVideoChannelListTimerId) {
		getVideoChannelList();
	} else if (timerEvent->timerId() == _everyDayTimerId) {
		everyDayActions();
	}
}

} // namespace Bettergrams
