#pragma once

#include <QObject>

namespace Bettergram {

class RssChannel;
class RssItem;

/**
 * @brief The RssChannelList class contains list of RssChannel instances.
 */
class RssChannelList : public QObject {
	Q_OBJECT

public:
	enum class NewsType {
		News,
		Videos
	};

	typedef QList<QSharedPointer<RssChannel>>::const_iterator const_iterator;

	explicit RssChannelList(NewsType newsType, QObject *parent);

	NewsType newsType() const;

	int freq() const;
	void setFreq(int freq);

	QDateTime lastUpdate() const;
	QString lastUpdateString() const;

	const_iterator begin() const;
	const_iterator end() const;

	/// Can throw std::out_of_range() exception
	const QSharedPointer<RssChannel> &at(int index) const;

	bool isEmpty() const;
	int count() const;
	int countAllItems() const;
	int countAllUnreadItems() const;

	void add(const QUrl &channelLink);
	bool contains(const QUrl &channelLink);

	void markAsRead();

	QList<QSharedPointer<RssItem>> getAllItems() const;
	QList<QSharedPointer<RssItem>> getAllUnreadItems() const;

	void load();
	void parseFeeds();
	void parseChannelList(const QByteArray &channelList);

public slots:

signals:
	void update();

	void freqChanged();
	void lastUpdateChanged();
	void iconChanged();
	
	void updated();

protected:

private:
	/// Default frequency of updates in seconds
	static const int _defaultFreq;

	QList<QSharedPointer<RssChannel>> _list;

	const NewsType _newsType;

	/// It is used for storing and loading data
	const QString _name;

	int _imageWidth = 0;
	int _imageHeight = 0;

	/// Frequency of updates in seconds
	int _freq;

	QDateTime _lastUpdate;
	QString _lastUpdateString;
	QByteArray _lastSourceHash;

	static QString getName(NewsType newsType);
	static int getImageWidth(NewsType newsType);
	static int getImageHeight(NewsType newsType);

	void setLastUpdate(const QDateTime &lastUpdate);
	void add(QSharedPointer<RssChannel> &channel);

	void parseChannelList(const QJsonObject &json);

	void save();

private slots:
	void onIsReadChanged();
};

} // namespace Bettergram
