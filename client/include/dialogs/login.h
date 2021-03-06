
#if !defined(__LOGIN_H__)
#define __LOGIN_H__

#include <QList>
#include <QHostAddress>
#include <QObject>
#include <QStringList>
#include <QString>

#include "gui/control.h"
#include "gui/window.h"
#include "gui/imagebutton.h"
#include "gui/textfield.h"
#include "gui/asciilabel.h"
#include "gui/label.h"

// Structure used for the shardlist
struct stShardEntry {
	QString name;
	QHostAddress pingAddress;
	unsigned char percentFull;
	unsigned char timezone;
	unsigned short id;
};

enum enMenuPage {
	PAGE_LOGIN = 0,
	PAGE_CONNECTING,
	PAGE_VERIFYING,
	PAGE_SHARDLIST,
	PAGE_SELECTCHAR,
	PAGE_CONFIRMDELETE,
	PAGE_DELETING,
	PAGE_ENTERING,
	PAGE_NEWCHARACTER
};

class cCharSelection;

class cLoginDialog : public QObject {
Q_OBJECT

private:
	cCharSelection *charSelectWidget;
	cWindow *container; // The main container
	cAsciiLabel *lastShardName;
	cImageButton *movieButton, *nextButton, *backButton;
	cContainer *accountLoginGump;
	cContainer *shardSelectGump;
	cTextField *inpAccount, *inpPassword;
	cContainer *shardList;
	cContainer *confirmDeleteDialog;
	cAsciiLabel *confirmDeleteText;
	cContainer *statusDialog;
	cContainer *selectCharDialog;
	cLabel *statusLabel;
	enMenuPage page;
	cImageButton *statusCancel, *statusOk;
	cControl *selectCharBorder[6];
	QStringList characterNames;

	void buildConfirmDeleteGump();
	void buildAccountLoginGump();
	void buildShardSelectGump();
	void buildStatusGump();
	void buildSelectCharGump();

	QList<stShardEntry> shards;
	unsigned int shardEntryOffset;
	bool errorStatus;
public:
	cLoginDialog();
	~cLoginDialog();

public slots:
	// Button callbacks for this page.
	void quitClicked(cControl *sender);
	void nextClicked(cControl *sender);
	void backClicked(cControl *sender);
	void myUoClicked(cControl *sender);
	void creditsClicked(cControl *sender);
	void accountClicked(cControl *sender);
    void movieClicked(cControl *sender);
	void helpClicked(cControl *sender);
	void deleteCharClicked(cControl *sender);
	void createCharClicked(cControl *sender);
	void statusCancelClicked(cControl *sender);
	void statusOkClicked(cControl *sender);
	void charSelected(cControl *sender);
	void shardlistScrolled(int oldpos);

	// These are connected to the uoSocket
	void socketError(const QString &error);
	void socketHostFound();
	void socketConnect();
	void selectShard(int id);
	void enterPressed(cTextField *field);
	void selectLastShard();

	void setStatusText(const QString &text);
	void setErrorStatus(bool error);

	void show(int page);
	void show(enMenuPage page); // Show the login dialog
	void hide(); // Hide the login dialog

	// Callback for the ShardList
	void clearShardList();
	void addShard(const stShardEntry &shard);
	void setCharacterList(const QStringList &characters);

signals:
	void showCharacterCreation();
	void hideCharacterCreation();
};

inline void cLoginDialog::show(int page) {
	show(enMenuPage(page));
}

extern cLoginDialog *LoginDialog; // There is only one LoginDialog instance at a time

#endif
