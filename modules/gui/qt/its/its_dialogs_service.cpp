#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "its_dialogs_service.hpp"

#include <QTextCursor>
#include <QTextEdit>
#include <QCheckBox>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMovie>

namespace its {

//=> BusyDialogs
BusyDialog::BusyDialog( intf_thread_t *_p_intf ): QDialog( (QWidget*)_p_intf->p_sys->p_mi )
, _movie( new QMovie( this ) )
, _movieLabel( new QLabel( this ) )
, _messageLabel( new QLabel( this ) )
{
    setWindowFlags( Qt::Dialog | Qt::FramelessWindowHint );

    setModal( true );
    setFixedSize( 150, 150 );
    setWindowTitle( qtr( "Busy" ) );
    setWindowRole( "vlc-errors" );

    QGridLayout *layout = new QGridLayout( this );

    layout->addWidget( _movieLabel, 0, 0, 1, 0, Qt::AlignCenter );
    layout->addWidget( _messageLabel, 1, 0, Qt::AlignCenter );

    QPixmap loadingPixmap( loadingGIF );

    _movie->setFileName( loadingGIF );
    _movie->setScaledSize( QSize(50, 50) );
    _movieLabel->setMovie( _movie );
    _movieLabel->setAttribute( Qt::WA_TranslucentBackground, true );
    _movie->start();

    _messageLabel->setText( _( "Please wait..." ) );
    QFont font( "Arial", 12, QFont::Normal );
    _messageLabel->setFont( font );
}

BusyDialog::~BusyDialog()
{
}

void BusyDialog::showDialog()
{
    show();
}

void BusyDialog::hideDialog()
{
    hide();
}

//<= BusyDialog

//=> DialogsService

DialogsService* DialogsService::_instance = NULL;
intf_thread_t* DialogsService::_p_intf = NULL;

DialogsService::DialogsService( intf_thread_t *p_intf )
{
    DialogsService::_p_intf = p_intf;
}

DialogsService::~DialogsService()
{
}

void DialogsService::showBusy()
{
    BusyDialog::getInstance( getInterfaceThread() )->showDialog();
}

void DialogsService::hideBusy()
{
    BusyDialog::getInstance( getInterfaceThread() )->hideDialog();
}

void DialogsService::displayError( QString title, QString error )
{
    ErrorsDialog::getInstance( getInterfaceThread() )->addError( title, error );
}

void DialogsService::displayQuestion( const QString &title, const QString &text,
    int i_type, const QString &cancel,
    const QString &action1,
    const QString &action2,
    Fn<void(int actionNumber)> onQuestionCallback )
{
    enum QMessageBox::Icon icon;
    switch ( i_type )
    {
        case DIALOG_QUESTION_WARNING:
            icon = QMessageBox::Warning;
            break;
        case DIALOG_QUESTION_CRITICAL:
            icon = QMessageBox::Critical;
            break;
        default:
        case DIALOG_QUESTION_NORMAL:
            icon = action1.isEmpty() && action2.isEmpty() ?
                 QMessageBox::Information : QMessageBox::Question;
            break;
    }
    QMessageBox *box = new QMessageBox( icon, title, text );
    box->addButton ( "&" + cancel, QMessageBox::RejectRole );
    box->setModal( true );
    QAbstractButton *action1Button = NULL;
    if ( !action1.isEmpty() )
        action1Button = box->addButton( "&" + action1, QMessageBox::AcceptRole );
    QAbstractButton *action2Button = NULL;
    if ( !action2.isEmpty() )
        action2Button = box->addButton( "&" + action2, QMessageBox::AcceptRole );

    connect( box, &QMessageBox::finished, this, [onQuestionCallback]( int result ) {
        onQuestionCallback( result );
    });

    box->show();
}

//<= DialogsService

} // its
