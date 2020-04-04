#ifndef VLC_QT_ITS_DIALOGS_HPP_
#define VLC_QT_ITS_DIALOGS_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>

#include <vlc_input.h>
#include <vlc_common.h>    /* VLC_COMMON_MEMBERS for vlc_interface.h */
#include <vlc_interface.h> /* intf_thread_t */
#include <vlc_playlist.h>  /* playlist_t */

#include "qt.hpp"

#include <QObject>
#include <QEvent>
#include <QEventLoop>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QMovie>

#include "dialogs/errors.hpp"
#include "util/qvlcframe.hpp"
#include "util/singleton.hpp"

#include "its/its_base.hpp"

namespace its {

//=> BusyDialogs
class BusyDialog : public QDialog, public Singleton<BusyDialog>
{
    Q_OBJECT
public:
	void showDialog();
	void hideDialog();
private:
    virtual ~BusyDialog();
    BusyDialog( intf_thread_t * );
	QMovie *_movie;
	QLabel *_movieLabel, *_messageLabel;

	const QString loadingGIF = ":/toolbar/its_loading.gif";
	friend class Singleton<BusyDialog>;
};
//<= BusyDialog

typedef enum its_dialog_question_type
{
    DIALOG_QUESTION_NORMAL,
    DIALOG_QUESTION_WARNING,
    DIALOG_QUESTION_CRITICAL,
} its_dialog_question_type;

class DialogsService : public QObject
{
	Q_OBJECT

public:
	static DialogsService *getInstance()
	{
		assert( _instance );
		return _instance;
	}
	static DialogsService *getInstance( intf_thread_t *p_intf )
	{
		if( !_instance )
			_instance = new DialogsService( p_intf );
		return _instance;
	}
	static void killInstance()
	{
		delete _instance;
		_instance = NULL;
		_p_intf = NULL;
	}

	static intf_thread_t *getInterfaceThread() {
		return _p_intf;
	}

	void showBusy();
	void hideBusy();
	void displayError( QString title, QString error );
	void displayQuestion( const QString &title, const QString &text, int i_type,
                         const QString &cancel, const QString &action1,
                         const QString &action2,
                         Fn<void(int actionNumber)> onQuestionCallback = Fn<void(int)>() );

private:

	DialogsService( intf_thread_t * p_intf );
	virtual ~DialogsService();

	static DialogsService *_instance;
	static intf_thread_t *_p_intf;
};

} // its

#endif // include-guard

