#include "cryptopricelist.h"
#include "cryptoprice.h"

#include <bettergram/bettergramservice.h>
#include <logs.h>

#include <QJsonDocument>
#include <QJsonObject>

namespace Bettergram {

const int CryptoPriceList::_defaultFreq = 60;
const int CryptoPriceList::_minimumSearchText = 2;

const QString &CryptoPriceList::getSortString(SortOrder sortOrder)
{
	static const QString rank = QStringLiteral("rank");
	static const QString name = QStringLiteral("name");
	static const QString price = QStringLiteral("price");
	static const QString delta = QStringLiteral("delta.day");

	switch (sortOrder) {
	case(SortOrder::Rank):
		return rank;
	case(SortOrder::NameAscending):
		return name;
	case(SortOrder::NameDescending):
		return name;
	case(SortOrder::PriceAscending):
		return price;
	case(SortOrder::PriceDescending):
		return price;
	case(SortOrder::ChangeFor24hAscending):
		return delta;
	case(SortOrder::ChangeFor24hDescending):
		return delta;
	default:
		LOG(("Can not recognize sort order value %1").arg(static_cast<int>(sortOrder)));
		return rank;
	}
}

const QString &CryptoPriceList::getOrderString(SortOrder sortOrder)
{
	static const QString ascending = QStringLiteral("ascending");
	static const QString descending = QStringLiteral("descending");

	switch (sortOrder) {
	case(SortOrder::Rank):
		return ascending;
	case(SortOrder::NameAscending):
		return ascending;
	case(SortOrder::NameDescending):
		return descending;
	case(SortOrder::PriceAscending):
		return ascending;
	case(SortOrder::PriceDescending):
		return descending;
	case(SortOrder::ChangeFor24hAscending):
		return ascending;
	case(SortOrder::ChangeFor24hDescending):
		return descending;
	default:
		LOG(("Can not recognize sort order value %1").arg(static_cast<int>(sortOrder)));
		return ascending;
	}
}

CryptoPriceList::CryptoPriceList(QObject *parent) :
	QObject(parent),
	_freq(_defaultFreq),
	_lastUpdateString(BettergramService::defaultLastUpdateString())
{
	updateMarketCapString();
	updateBtcDominanceString();
}

std::optional<double> CryptoPriceList::marketCap() const
{
	return _marketCap;
}

const QString &CryptoPriceList::marketCapString() const
{
	return _marketCapString;
}

void CryptoPriceList::setMarketCap(const std::optional<double> &marketCap)
{
	if (_marketCap != marketCap) {
		_marketCap = marketCap;

		updateMarketCapString();
		emit marketCapChanged();
	}
}

void CryptoPriceList::updateMarketCapString()
{
	if (!_marketCap) {
		_marketCapString = QStringLiteral("N/A");
		return;
	}

	qint64 value = qAbs(qRound64(*_marketCap));

	if (!value) {
		_marketCapString = "0";
		return;
	}

	_marketCapString.clear();

	while (true) {
		qint64 temp = value % 1000;
		value /= 1000;

		if (!_marketCapString.isEmpty()) {
			_marketCapString.prepend(",");
		}

		_marketCapString.prepend(QString("%1").arg(temp, 3, 10, QLatin1Char('0')));

		if (!value) {
			break;
		}
	}

	_marketCapString.prepend(QStringLiteral("$"));
}

std::optional<double> CryptoPriceList::btcDominance() const
{
	return _btcDominance;
}

const QString &CryptoPriceList::btcDominanceString() const
{
	return _btcDominanceString;
}

void CryptoPriceList::setBtcDominance(const std::optional<double> &btcDominance)
{
	if (_btcDominance != btcDominance) {
		_btcDominance = btcDominance;

		updateBtcDominanceString();
		emit btcDominanceChanged();
	}
}

void CryptoPriceList::updateBtcDominanceString()
{
	if (!_btcDominance) {
		_btcDominanceString = QStringLiteral("N/A");
		return;
	}

	if (qAbs(*_btcDominance) < 1.0) {
		_btcDominanceString = QStringLiteral("%1%").arg(*_btcDominance, 0, 'f', 4);
	} else {
		_btcDominanceString = QStringLiteral("%1%").arg(*_btcDominance, 0, 'f', 2);
	}
}

void CryptoPriceList::updateFavoriteList()
{
	if (_isShowOnlyFavorites) {
		_favoriteList.clear();

		for (const QSharedPointer<CryptoPrice> &price : _list) {
			if (price->isFavorite()) {
				_favoriteList.push_back(price);
			}
		}
	} else {
		_favoriteList.clear();
	}
}

int CryptoPriceList::freq() const
{
	return _freq;
}

void CryptoPriceList::setFreq(int freq)
{
	if (freq <= 0) {
		freq = _defaultFreq;
	}

	if (_freq != freq) {
		_freq = freq;
		emit freqChanged();
	}
}

QDateTime CryptoPriceList::lastUpdate() const
{
	return _lastUpdate;
}

QString CryptoPriceList::lastUpdateString() const
{
	return _lastUpdateString;
}

void CryptoPriceList::setLastUpdate(const QDateTime &lastUpdate)
{
	if (_lastUpdate != lastUpdate) {
		_lastUpdate = lastUpdate;

		_lastUpdateString = BettergramService::generateLastUpdateString(_lastUpdate, true);
	}
}

void CryptoPriceList::addPrivate(const QSharedPointer<CryptoPrice> &price)
{
	connect(price.data(), &CryptoPrice::iconChanged,
			this, &CryptoPriceList::onIconChanged);

	connect(price.data(), &CryptoPrice::isFavoriteToggled,
			this, &CryptoPriceList::onIsFavoriteToggled);

	_list.push_back(price);
}

QSharedPointer<CryptoPrice> CryptoPriceList::at(int index) const
{
	if (index < 0 || index >= count()) {
		LOG(("Index is out of bounds"));
		return QSharedPointer<CryptoPrice>(nullptr);
	}

	if (isSearching()) {
		return _searchList.at(index);
	} else if (_isShowOnlyFavorites) {
		return _favoriteList.at(index);
	} else {
		return _list.at(index);
	}

}

int CryptoPriceList::count() const
{
	if (isSearching()) {
		return _searchList.count();
	} else if (_isShowOnlyFavorites) {
		return _favoriteList.count();
	} else if (_lastListValuesTotalCount > 0) {
		return qMin(_lastListValuesTotalCount, _list.count());
	} else {
		return _list.count();
	}
}

QList<QSharedPointer<CryptoPrice>> CryptoPriceList::searchList() const
{
	return _searchList;
}

QList<QSharedPointer<CryptoPrice>> CryptoPriceList::favoriteList() const
{
	return _favoriteList;
}

CryptoPriceList::SortOrder CryptoPriceList::sortOrder() const
{
	return _sortOrder;
}

void CryptoPriceList::setSortOrder(const SortOrder &sortOrder)
{
	if (_sortOrder != sortOrder) {
		_sortOrder = sortOrder;

		emit sortOrderChanged();
	}
}

const QString &CryptoPriceList::sortString()
{
	return getSortString(_sortOrder);
}

const QString &CryptoPriceList::orderString()
{
	return getOrderString(_sortOrder);
}

bool CryptoPriceList::isSearching() const
{
	return _searchText.size() >= _minimumSearchText;
}

bool CryptoPriceList::isSearchInProgress() const
{
	return _isSearchInProgress;
}

const QString &CryptoPriceList::searchText() const
{
	return _searchText;
}

void CryptoPriceList::setSearchText(const QString &searchText)
{
	QString trimmedSearchText = searchText.trimmed();

	if (_searchText != trimmedSearchText) {
		bool wasSearching = isSearching();

		_searchText = trimmedSearchText;

		_isSearchInProgress = isSearching();

		if (wasSearching != isSearching()) {
			emit isSearchingChanged();
		}

		emit searchTextChanged();
	}
}

bool CryptoPriceList::isShowOnlyFavorites() const
{
	return _isShowOnlyFavorites;
}

void CryptoPriceList::setIsShowOnlyFavorites(bool isShowOnlyFavorites)
{
	if (_isShowOnlyFavorites != isShowOnlyFavorites) {
		_isShowOnlyFavorites = isShowOnlyFavorites;

		updateFavoriteList();

		emit isShowOnlyFavoritesChanged();
	}
}

void CryptoPriceList::toggleIsShowOnlyFavorites()
{
	setIsShowOnlyFavorites(!_isShowOnlyFavorites);
}

bool CryptoPriceList::areNamesFetched() const
{
	return _areNamesFetched;
}

bool CryptoPriceList::mayFetchStats() const
{
	return !_statsLastUpdate.isValid()
			|| (qAbs(_statsLastUpdate.secsTo(QDateTime::currentDateTime())) >= _freq / 4);
}

QStringList CryptoPriceList::getFavoritesShortNames() const
{
	QStringList result;

	for (const QSharedPointer<CryptoPrice> &coin : _favoriteList) {
		result.push_back(coin->shortName());
	}

	return result;
}

QStringList CryptoPriceList::getSearchListShortNames() const
{
	QStringList result;

	for (const QSharedPointer<CryptoPrice> &coin : _searchList) {
		result.push_back(coin->shortName());
	}

	return result;
}

QStringList CryptoPriceList::getSearchListShortNames(int offset, int count) const
{
	QStringList result;

	for (int i = qMax(0, offset); i < qMin(_searchList.size(), offset + count); ++i) {
		result.push_back(_searchList.at(i)->shortName());
	}

	return result;
}

void CryptoPriceList::setAreNamesFetched(bool areNamesFetched)
{
	if (_areNamesFetched != areNamesFetched) {
		_areNamesFetched = areNamesFetched;
	}
}

void CryptoPriceList::parseNames(const QByteArray &byteArray)
{
	setAreNamesFetched(false);

	if (byteArray.isEmpty()) {
		LOG(("Can not get crypto price names. Response is emtpy"));
		return;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

	if (!doc.isObject()) {
		LOG(("Can not get crypto price names. Response is wrong. %1 (%2). Response: %3")
			.arg(parseError.errorString())
			.arg(parseError.error)
			.arg(QString::fromUtf8(byteArray)));
		return;
	}

	QJsonObject json = doc.object();

	if (json.isEmpty()) {
		LOG(("Can not get crypto price names. Response is emtpy or wrong"));
		return;
	}

	bool success = json.value("success").toBool();

	if (!success) {
		QString errorMessage = json.value("message").toString("Unknown error");
		LOG(("Can not get crypto price names. %1").arg(errorMessage));
		return;
	}

	QString coinsUrlBase = json.value("coinsUrlBase").toString();

	// Now we fetch only 32x32 icons, but maybe in the future we will use 64x64 icons
	QString coinsIconBase = json.value("coinsIcon32Base").toString();

	QJsonArray priceListJson = json.value("data").toArray();

	if (priceListJson.isEmpty()) {
		LOG(("Can not get crypto price names. The 'data' list is empty"));
		return;
	}

	QList<CryptoPrice> priceList;
	bool isAdded = false;

	for (QJsonValue jsonValue : priceListJson) {
		QJsonObject priceJson = jsonValue.toObject();

		if (priceJson.isEmpty()) {
			LOG(("Price json is empty"));
			continue;
		}

		const QString type = priceJson.value("type").toString();

		if (type != QStringLiteral("coin")) {
			continue;
		}

		const QString name = priceJson.value("name").toString();
		if (name.isEmpty()) {
			continue;
		}

		const QString shortName = priceJson.value("code").toString();
		if (shortName.isEmpty()) {
			continue;
		}

		QString url = priceJson.value("url").toString();
		if (url.isEmpty()) {
			if (coinsUrlBase.isEmpty()) {
				continue;
			}

			url = coinsUrlBase
					+ QString(name).remove(QRegExp(QStringLiteral("\\W")))
					+ QStringLiteral("-")
					+ QString(shortName).remove(QRegExp(QStringLiteral("\\W")));
		}

		QString iconUrl = priceJson.value("icon").toString();
		if (iconUrl.isEmpty()) {
			continue;
		}

		if (!coinsIconBase.isEmpty()) {
			iconUrl = coinsIconBase + iconUrl;
		}

		CryptoPrice cryptoPrice(url, iconUrl, name, shortName, false);

		priceList.push_back(cryptoPrice);

		isAdded = true;
	}

	mergeCryptoPriceList(priceList);
	updateFavoriteList();

	if (!_list.isEmpty() && isAdded) {
		setAreNamesFetched(true);
	}

	if (_areNamesFetched) {
		emit namesUpdated();
	}
}

void CryptoPriceList::parseSearchNames(const QByteArray &byteArray)
{
	_searchList.clear();

	if (!isSearching()) {
		searchResultsAreEmpty();
		return;
	}

	if (byteArray.isEmpty()) {
		LOG(("Can not search crypto price names. Response is emtpy"));

		searchResultsAreEmpty();
		return;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

	if (!doc.isObject()) {
		LOG(("Can not search crypto price names. Response is wrong. %1 (%2). Response: %3")
			.arg(parseError.errorString())
			.arg(parseError.error)
			.arg(QString::fromUtf8(byteArray)));

		searchResultsAreEmpty();
		return;
	}

	QJsonObject json = doc.object();

	if (json.isEmpty()) {
		LOG(("Can not search crypto price names. Response is emtpy or wrong"));

		searchResultsAreEmpty();
		return;
	}

	bool success = json.value("success").toBool();

	if (!success) {
		QString errorMessage = json.value("message").toString("Unknown error");
		LOG(("Can not search crypto price names. %1").arg(errorMessage));

		searchResultsAreEmpty();
		return;
	}

	QString coinsUrlBase = json.value("coinsUrlBase").toString();

	// Now we fetch only 32x32 icons, but maybe in the future we will use 64x64 icons
	QString coinsIconBase = json.value("coinsIcon32Base").toString();

	QJsonArray priceListJson = json.value("data").toArray();

	if (priceListJson.isEmpty()) {
		searchResultsAreEmpty();
		return;
	}

	for (QJsonValue jsonValue : priceListJson) {
		QJsonObject priceJson = jsonValue.toObject();

		if (priceJson.isEmpty()) {
			LOG(("Price json is empty"));
			continue;
		}

		const QString type = priceJson.value("type").toString();

		if (type != QStringLiteral("coin")) {
			continue;
		}

		const QString name = priceJson.value("name").toString();
		if (name.isEmpty()) {
			continue;
		}

		const QString shortName = priceJson.value("code").toString();
		if (shortName.isEmpty()) {
			continue;
		}

		QSharedPointer<CryptoPrice> price = findByName(name, shortName);

		if (!price) {
			QString url = priceJson.value("url").toString();
			if (url.isEmpty()) {
				if (coinsUrlBase.isEmpty()) {
					continue;
				}

				url = coinsUrlBase
						+ QString(name).remove(QRegExp(QStringLiteral("\\W")))
						+ QStringLiteral("-")
						+ QString(shortName).remove(QRegExp(QStringLiteral("\\W")));
			}

			QString iconUrl = priceJson.value("icon").toString();
			if (iconUrl.isEmpty()) {
				continue;
			}

			if (!coinsIconBase.isEmpty()) {
				iconUrl = coinsIconBase + iconUrl;
			}

			price = QSharedPointer<CryptoPrice>(new CryptoPrice(url,
																iconUrl,
																name,
																shortName,
																false));

			addPrivate(price);
		}

		_searchList.push_back(price);
	}

	_isSearchInProgress = false;

	if (_list.isEmpty()) {
		searchResultsAreEmpty();
	} else {
		emit searchNamesUpdated();
	}
}

void CryptoPriceList::searchResultsAreEmpty()
{
	_isSearchInProgress = false;
	emit valuesUpdated(QUrl(), QList<QSharedPointer<CryptoPrice>>());
}

void CryptoPriceList::parseValues(const QByteArray &byteArray, const QUrl &url)
{
	if (byteArray.isEmpty()) {
		LOG(("Can not get crypto price values. Response is emtpy"));
		return;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

	if (!doc.isObject()) {
		LOG(("Can not get crypto price values. Response is wrong. %1 (%2). Response: %3")
			.arg(parseError.errorString())
			.arg(parseError.error)
			.arg(QString::fromUtf8(byteArray)));
		return;
	}

	QJsonObject json = doc.object();

	if (json.isEmpty()) {
		LOG(("Can not get crypto price values. Response is emtpy or wrong"));
		return;
	}

	bool success = json.value("success").toBool();

	if (!success) {
		QString errorMessage = json.value("message").toString("Unknown error");
		LOG(("Can not get crypto price values. %1").arg(errorMessage));
		return;
	}

	_lastListValuesTotalCount = json.value("total").toInt();

	QList<QSharedPointer<CryptoPrice>> prices;
	QJsonArray priceListJson = json.value("data").toArray();
	prices = parsePriceListValues(priceListJson);

	if (!isSearching() && _sortOrder != SortOrder::Rank) {
		sort(prices);
	}

	for (const QSharedPointer<CryptoPrice> &price : prices) {
		price->downloadIconIfNeeded();
	}

	setLastUpdate(QDateTime::currentDateTime());
	emit valuesUpdated(url, prices);
}

void CryptoPriceList::parseStats(const QByteArray &byteArray)
{
	if (byteArray.isEmpty()) {
		LOG(("Can not get crypto price stats. Response is emtpy"));
		return;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(byteArray, &parseError);

	if (!doc.isObject()) {
		LOG(("Can not get crypto price stats. Response is wrong. %1 (%2). Response: %3")
			.arg(parseError.errorString())
			.arg(parseError.error)
			.arg(QString::fromUtf8(byteArray)));
		return;
	}

	QJsonObject json = doc.object();

	if (json.isEmpty()) {
		LOG(("Can not get crypto price stats. Response is emtpy or wrong"));
		return;
	}

	bool success = json.value("success").toBool();

	if (!success) {
		QString errorMessage = json.value("message").toString("Unknown error");
		LOG(("Can not get crypto price stats. %1").arg(errorMessage));
		return;
	}

	double marketCap = json.value("cap").toDouble();
	double btcDominance = json.value("btcDominance").toDouble();

	// It is optionally parameter.
	// This parameter may contain number of seconds for the next update
	// (5, 60, 90 seconds and etc.).
	int freq = qAbs(json.value("freq").toInt());

	setMarketCap(marketCap);
	setBtcDominance(btcDominance);
	setFreq(freq);

	_statsLastUpdate = QDateTime::currentDateTime();

	emit statsUpdated();
}

void CryptoPriceList::emptyValues()
{
	emit valuesUpdated(QUrl(), QList<QSharedPointer<CryptoPrice>>());
}

QList<QSharedPointer<CryptoPrice>> CryptoPriceList::parsePriceListValues(const QJsonArray &priceListJson)
{
	QList<QSharedPointer<CryptoPrice>> prices;
	int i = 0;

	for (QJsonValue jsonValue : priceListJson) {
		QJsonObject priceJson = jsonValue.toObject();

		if (priceJson.isEmpty()) {
			LOG(("Price json is empty"));
			continue;
		}

		QString shortName = priceJson.value("code").toString();
		if (shortName.isEmpty()) {
			LOG(("Price code is empty"));
			continue;
		}

		QString name = priceJson.value("name").toString();
		if (name.isEmpty()) {
			LOG(("Price name is empty"));
			continue;
		}

		QJsonObject deltaJson = priceJson.value("delta").toObject();

		if (deltaJson.isEmpty()) {
			LOG(("Price delta is empty"));
			continue;
		}

		int rank = priceJson.contains("rank") ? priceJson.value("rank").toInt() : i;

		std::optional<double> currentPrice = std::nullopt;

		if (priceJson.contains("price") && priceJson.value("price").isDouble()) {
			currentPrice = priceJson.value("price").toDouble();
		}

		std::optional<double> changeFor24Hours = std::nullopt;

		if (deltaJson.contains("day") && deltaJson.value("day").isDouble()) {
			changeFor24Hours = (deltaJson.value("day").toDouble() - 1) * 100;
		}

		std::optional<double> changeForMinute = std::nullopt;

		if (deltaJson.contains("day") && deltaJson.value("minute").isDouble()) {
			changeForMinute = (deltaJson.value("minute").toDouble() - 1) * 100;
		}

		QSharedPointer<CryptoPrice> price = findByName(name, shortName);

		if (!price) {
			LOG(("Can not find price for crypto currency '%1.%2'").arg(name, shortName));
			continue;
		}

		price->setRank(rank);
		price->setCurrentPrice(currentPrice);
		price->setChangeFor24Hours(changeFor24Hours);
		price->setMinuteDirection(CryptoPrice::countDirection(changeForMinute));

		if (isSearching()) {
			if (_searchList.contains(price)) {
				prices.push_back(price);
			}
		} else if ((_isShowOnlyFavorites && price->isFavorite()) || !_isShowOnlyFavorites) {
			prices.push_back(price);
		}

		i++;
	}

	return prices;
}

void CryptoPriceList::save() const
{
	QSettings settings(BettergramService::instance()->pricesCacheSettingsPath(), QSettings::IniFormat);

	settings.beginGroup("metadata");

	if (marketCap()) {
		settings.setValue("marketCap", *marketCap());
	} else {
		settings.remove("marketCap");
	}

	if (btcDominance()) {
		settings.setValue("btcDominance", *btcDominance());
	} else {
		settings.remove("btcDominance");
	}

	settings.setValue("lastUpdate", lastUpdate());
	settings.setValue("isShowOnlyFavorites", isShowOnlyFavorites());

	if (freq() == _defaultFreq) {
		settings.remove("freq");
	} else {
		settings.setValue("freq", freq());
	}

	settings.endGroup();

	settings.beginGroup("prices");
	settings.beginWriteArray("prices", _list.size());

	for (int i = 0; i < _list.size(); ++i) {
		settings.setArrayIndex(i);
		_list.at(i)->save(settings);
	}

	settings.endArray();
	settings.endGroup();
}

void CryptoPriceList::load()
{
	QSettings settings(BettergramService::instance()->pricesCacheSettingsPath(), QSettings::IniFormat);

	settings.beginGroup("metadata");

	if (settings.contains("marketCap")) {
		setMarketCap(settings.value("marketCap").toDouble());
	} else {
		setMarketCap(std::nullopt);
	}

	if (settings.contains("btcDominance")) {
		setBtcDominance(settings.value("btcDominance").toDouble());
	} else {
		setBtcDominance(std::nullopt);
	}

	setFreq(qAbs(settings.value("freq").toInt()));
	setLastUpdate(settings.value("lastUpdate").toDateTime());
	setIsShowOnlyFavorites(settings.value("isShowOnlyFavorites", false).toBool());

	settings.endGroup();

	settings.beginGroup("prices");
	int size = settings.beginReadArray("prices");

	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);

		QSharedPointer<CryptoPrice> price = CryptoPrice::load(settings);

		if (price) {
			addPrivate(price);
		}
	}

	settings.endArray();
	settings.endGroup();

	updateFavoriteList();
}

void CryptoPriceList::mergeCryptoPriceList(const QList<CryptoPrice> &priceList)
{
	// Remove old crypto prices
	for (iterator it = _list.begin(); it != _list.end();) {
		const QSharedPointer<CryptoPrice> &price = *it;

		if (containsName(priceList, price->name(), price->shortName())) {
			++it;
		} else {
			it = _list.erase(it);
		}
	}

	// Update existed crypto prices and add new ones
	for (const CryptoPrice &price : priceList) {
		QSharedPointer<CryptoPrice> existedPrice = findByName(price.name(), price.shortName());

		if (existedPrice) {
			existedPrice->updateData(price);
		} else {
			QSharedPointer<CryptoPrice> newPrice(new CryptoPrice(price, this));
			newPrice->loadIsFavorite();

			addPrivate(newPrice);
		}
	}
}

QSharedPointer<CryptoPrice> CryptoPriceList::find(const CryptoPrice *pricePointer)
{
	for (const QSharedPointer<CryptoPrice> &price : _list) {
		if (price.data() == pricePointer) {
			return price;
		}
	}

	return QSharedPointer<CryptoPrice>(nullptr);
}

QSharedPointer<CryptoPrice> CryptoPriceList::findByName(const QString &name, const QString &shortName)
{
	for (const QSharedPointer<CryptoPrice> &price : _list) {
		if (price->name() == name && price->shortName() == shortName) {
			return price;
		}
	}

	return QSharedPointer<CryptoPrice>(nullptr);
}

QSharedPointer<CryptoPrice> CryptoPriceList::findByShortName(const QString &shortName)
{
	for (const QSharedPointer<CryptoPrice> &price : _list) {
		if (price->shortName() == shortName) {
			return price;
		}
	}

	return QSharedPointer<CryptoPrice>(nullptr);
}

bool CryptoPriceList::containsName(const QList<CryptoPrice> &priceList,
								   const QString &name,
								   const QString &shortName)
{
	for (const CryptoPrice &price : priceList) {
		if (price.name() == name && price.shortName() == shortName) {
			return true;
		}
	}

	return false;
}

bool CryptoPriceList::containsShortName(const QList<QSharedPointer<CryptoPrice> > &priceList,
										const QString &shortName)
{
	for (const QSharedPointer<CryptoPrice> &price : priceList) {
		if (price->shortName() == shortName) {
			return true;
		}
	}

	return false;
}

bool CryptoPriceList::sortByRankAsc(const QSharedPointer<CryptoPrice> &price1,
									const QSharedPointer<CryptoPrice> &price2)
{
	if (price1->rank() == 0 && price2->rank() == 0) {
		return sortByNameAsc(price1, price2);
	}

	if (price1->rank() == 0) {
		return false;
	}

	if (price2->rank() == 0) {
		return true;
	}

	if (price1->rank() == price2->rank()) {
		return sortByNameAsc(price1, price2);
	}

	return price1->rank() < price2->rank();
}

bool CryptoPriceList::sortByRankDesc(const QSharedPointer<CryptoPrice> &price1,
									 const QSharedPointer<CryptoPrice> &price2)
{
	if (price1->rank() == 0 && price2->rank() == 0) {
		return sortByNameAsc(price1, price2);
	}

	if (price1->rank() == 0) {
		return false;
	}

	if (price2->rank() == 0) {
		return true;
	}

	if (price1->rank() == price2->rank()) {
		return sortByNameAsc(price1, price2);
	}

	return price2->rank() < price1->rank();
}

bool CryptoPriceList::sortByNameAsc(const QSharedPointer<CryptoPrice> &price1,
									const QSharedPointer<CryptoPrice> &price2)
{
	return QString::compare(price1->name(), price2->name(), Qt::CaseInsensitive) < 0;
}

bool CryptoPriceList::sortByNameDesc(const QSharedPointer<CryptoPrice> &price1,
									 const QSharedPointer<CryptoPrice> &price2)
{
	return QString::compare(price2->name(), price1->name(), Qt::CaseInsensitive) < 0;
}

bool CryptoPriceList::sortByPriceAsc(const QSharedPointer<CryptoPrice> &price1,
									 const QSharedPointer<CryptoPrice> &price2)
{
	return sortByDoubleAsc(price1, price2, price1->currentPrice(), price2->currentPrice());
}

bool CryptoPriceList::sortByPriceDesc(const QSharedPointer<CryptoPrice> &price1,
									  const QSharedPointer<CryptoPrice> &price2)
{
	return sortByDoubleDesc(price1, price2, price1->currentPrice(), price2->currentPrice());
}

bool CryptoPriceList::sortBy24hAsc(const QSharedPointer<CryptoPrice> &price1,
								   const QSharedPointer<CryptoPrice> &price2)
{
	return sortByDoubleAsc(price1, price2, price1->changeFor24Hours(), price2->changeFor24Hours());
}

bool CryptoPriceList::sortBy24hDesc(const QSharedPointer<CryptoPrice> &price1,
									const QSharedPointer<CryptoPrice> &price2)
{
	return sortByDoubleDesc(price1, price2, price1->changeFor24Hours(), price2->changeFor24Hours());
}

bool CryptoPriceList::sortByDoubleAsc(const QSharedPointer<CryptoPrice> &price1,
									  const QSharedPointer<CryptoPrice> &price2,
									  const std::optional<double> &value1,
									  const std::optional<double> &value2)
{
	if (!value1 && !value2) {
		return sortByNameAsc(price1, price2);
	}

	if (!value1) {
		return false;
	}

	if (!value2) {
		return true;
	}

	if (*value1 == *value2) {
		return sortByNameAsc(price1, price2);
	}

	return *value1 < *value2;
}

bool CryptoPriceList::sortByDoubleDesc(const QSharedPointer<CryptoPrice> &price1,
									   const QSharedPointer<CryptoPrice> &price2,
									   const std::optional<double> &value1,
									   const std::optional<double> &value2)
{
	if (!value1 && !value2) {
		return sortByNameAsc(price1, price2);
	}

	if (!value1) {
		return false;
	}

	if (!value2) {
		return true;
	}

	if (*value1 == *value2) {
		return sortByNameAsc(price1, price2);
	}

	return *value2 < *value1;
}

void CryptoPriceList::sort(QList<QSharedPointer<CryptoPrice>> &list)
{
	switch (_sortOrder) {
	case SortOrder::Rank:
		std::sort(list.begin(), list.end(), &CryptoPriceList::sortByRankAsc);
		break;
	case SortOrder::NameAscending:
		std::sort(list.begin(), list.end(), &CryptoPriceList::sortByNameAsc);
		break;
	case SortOrder::NameDescending:
		std::sort(list.begin(), list.end(), &CryptoPriceList::sortByNameDesc);
		break;
	case SortOrder::PriceAscending:
		std::sort(list.begin(), list.end(), &CryptoPriceList::sortByPriceAsc);
		break;
	case SortOrder::PriceDescending:
		std::sort(list.begin(), list.end(), &CryptoPriceList::sortByPriceDesc);
		break;
	case SortOrder::ChangeFor24hAscending:
		std::sort(list.begin(), list.end(), &CryptoPriceList::sortBy24hAsc);
		break;
	case SortOrder::ChangeFor24hDescending:
		std::sort(list.begin(), list.end(), &CryptoPriceList::sortBy24hDesc);
		break;
	default:
		break;
	}
}

void CryptoPriceList::createTestData()
{
	clear();

	addTestData(QUrl("https://www.livecoinwatch.com/price/Bitcoin-BTC"), QUrl("https://www.livecoinwatch.com/images/icons32/btc.png"), "Bitcoin", "BTC", 0, 7935.96, -3.22, CryptoPrice::Direction::Down);
	addTestData(QUrl("https://www.livecoinwatch.com/price/Ethereum-ETH"), QUrl("https://www.livecoinwatch.com/images/icons32/eth.png"), "Ethereum", "ETH", 3, 625.64, -8.43, CryptoPrice::Direction::Down);
	addTestData(QUrl("https://www.livecoinwatch.com/price/Ripple-XRP"), QUrl("https://www.livecoinwatch.com/images/icons32/xrp.png"), "Ripple", "XRP", 2, 0.6246, -6.59, CryptoPrice::Direction::Up);
	addTestData(QUrl("https://www.livecoinwatch.com/price/BitcoinCash-BCH"), QUrl("https://www.livecoinwatch.com/images/icons32/bch.png"), "Bitcoin Cash", "BCH", 1, 10640.2856, 9.45, CryptoPrice::Direction::Up);
	addTestData(QUrl("https://www.livecoinwatch.com/price/EOS-EOS"), QUrl("https://www.livecoinwatch.com/images/icons32/eos.png"), "EOS", "EOS", 4, 11.5, -100.6358, CryptoPrice::Direction::None);
	addTestData(QUrl("https://www.livecoinwatch.com/price/Litecoin-LTC"), QUrl("https://www.livecoinwatch.com/images/icons32/ltc.png"), "Litecoin", "LTC", 5, 125.46, -5.28, CryptoPrice::Direction::Up);
	addTestData(QUrl("https://www.livecoinwatch.com/price/Cardano-ADA"), QUrl("https://www.livecoinwatch.com/images/icons32/ada.png"), "Cardano", "ADA", 6, 0.2151, -9.72, CryptoPrice::Direction::None);
	addTestData(QUrl("https://www.livecoinwatch.com/price/Stellar-XLM"), QUrl("https://www.livecoinwatch.com/images/icons32/xlm.png"), "Stellar", "XLM", 7, 0.2882, -6.81, CryptoPrice::Direction::None);
	addTestData(QUrl("https://www.livecoinwatch.com/price/TRON-TRX"), QUrl("https://www.livecoinwatch.com/images/icons32/trx.png"), "TRON", "TRX", 8, 0.071258, -8.4789, CryptoPrice::Direction::Down);
	addTestData(QUrl("https://www.livecoinwatch.com/price/IOTA-MIOTA"), QUrl("https://www.livecoinwatch.com/images/icons32/miota.png"), "IOTA", "MIOTA", 9, 1.55, 8.77, CryptoPrice::Direction::Up);
}

void CryptoPriceList::addTestData(const QUrl &url,
								  const QUrl &iconUrl,
								  const QString &name,
								  const QString &shortName,
								  int rank,
								  double currentPrice,
								  double changeFor24Hours,
								  CryptoPrice::Direction minuteDirection)
{
	_list.push_back(QSharedPointer<CryptoPrice>(new CryptoPrice(url,
												   iconUrl,
												   name,
												   shortName,
												   rank,
												   currentPrice,
												   changeFor24Hours,
												   minuteDirection,
												   true,
												   this)));
}

void CryptoPriceList::onIconChanged()
{
	emit valuesUpdated(QUrl(), QList<QSharedPointer<CryptoPrice>>());
}

void CryptoPriceList::onIsFavoriteToggled()
{
	const CryptoPrice *pricePointer = qobject_cast<const CryptoPrice*>(sender());

	if (!pricePointer) {
		return;
	}

	QSharedPointer<CryptoPrice> price = find(pricePointer);

	if (price.isNull()) {
		return;
	}

	if (price->isFavorite()) {
		_favoriteList.push_back(price);
	} else {
		_favoriteList.removeAll(price);
	}
}

void CryptoPriceList::clear()
{
	_list.clear();
}

} // namespace Bettergrams
