#ifndef KRVFSMODEL_H
#define KRVFSMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QFont>

#include "krview.h"

class vfs;
class vfile;
class KrViewProperties;
class KrView;

class KrVfsModel: public QAbstractListModel {
	Q_OBJECT
	
public:
	enum ColumnType { Name = 0x0, Extension = 0x1, Size = 0x2, Mime = 0x3, DateTime = 0x4,
                          Permissions = 0x5, KrPermissions = 0x6, Owner = 0x7, Group = 0x8, MAX_COLUMNS = 0x09 };
	
	KrVfsModel( KrView * );
	virtual ~KrVfsModel();
	
	inline bool ready() const { return _ready; }
	void setVfs(vfs* v, bool upDir);
	QModelIndex addItem( vfile * );
	QModelIndex removeItem( vfile * );
	void updateItem( vfile * );
	
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	void setExtensionEnabled( bool exten ) { _extensionEnabled = exten; }
	inline const KrViewProperties * properties() const { return _view->properties(); }
	void sort() { sort( _lastSortOrder, _lastSortDir ); }
	void clear();
	vfile * vfileAt( const QModelIndex &index );
	vfile *dummyVfile() const { return _dummyVfile; }
	const QModelIndex & vfileIndex( vfile * );
	const QModelIndex & nameIndex( const QString & );
	virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
	static QString krPermissionString( const vfile * vf );
	void emitChanged() { emit layoutChanged(); }
	int convertSortOrderFromKrViewProperties( KrViewProperties::SortSpec, Qt::SortOrder & );
	KrViewProperties::SortSpec convertSortOrderToKrViewProperties( int, Qt::SortOrder );
	
	Qt::SortOrder getLastSortDir() { return _lastSortDir; }
	int getLastSortOrder() { return _lastSortOrder; }
	void setAlternatingTable( bool altTable ) { _alternatingTable = altTable; }
	
public slots:
	virtual void sort ( int column, Qt::SortOrder order = Qt::AscendingOrder );
	
protected:
	QString nameWithoutExtension( const vfile * vf, bool checkEnabled = true ) const;
	
	
	QVector<vfile*>             _vfiles;
	QHash<vfile *, QModelIndex> _vfileNdx;
	QHash<QString, QModelIndex> _nameNdx;
	bool                        _extensionEnabled;
	KrView                    * _view;
	int                         _lastSortOrder;
	Qt::SortOrder               _lastSortDir;
	vfile *                     _dummyVfile;
	bool                        _ready;
	QFont                       _defaultFont;
	bool                        _justForSizeHint;
	int                         _fileIconSize;
	bool                        _alternatingTable;
};
#endif // __krvfsmodel__
