#include "DlgCreatePeer.h"
#include "ui_DlgCreatePeer.h"
#include "SettingsManager.h"
#include "NotificationObserver.h"
#include "TrayControlWindow.h"
#include "DlgNotification.h"
#include "QDir"
#include "QStandardPaths"

DlgCreatePeer::DlgCreatePeer(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::DlgCreatePeer)
{

    ui->setupUi(this);
    connect(ui->btn_create, &QPushButton::clicked, this, &DlgCreatePeer::create_button_pressed);
}

DlgCreatePeer::~DlgCreatePeer()
{
  delete ui;
}

void DlgCreatePeer::create_button_pressed(){
    ui->btn_create->setEnabled(false);

    QString name = "subutai-peer_";
    name += ui->le_name->text();
    QString ram = ui->le_ram->text();
    QString cpu = ui->cmb_cpu->currentText();
    QString os = ui->cmb_os->currentText();

    QRegExp check_ram("\\d*");

    if(name.isEmpty()){
        CNotificationObserver::Error(tr("Name can't be empty"), DlgNotification::N_NO_ACTION);
        ui->btn_create->setEnabled(true);
        return;
    }

    if(name.contains(" ")){
        CNotificationObserver::Error(tr("Name can't have space"), DlgNotification::N_NO_ACTION);
        ui->btn_create->setEnabled(true);
        return;
    }
    if(!check_ram.exactMatch(ram)){
        CNotificationObserver::Error(tr("Ram shold be integer"), DlgNotification::N_NO_ACTION);
        ui->btn_create->setEnabled(true);
        return;
    }
    if(ram.isEmpty()){
        CNotificationObserver::Error(tr("Ram can't be empty"), DlgNotification::N_NO_ACTION);
        ui->btn_create->setEnabled(true);
        return;
    }

    QString dir = create_dir(name);

    if(dir.isEmpty()){
        CNotificationObserver::Error(tr("Name already exists"), DlgNotification::N_NO_ACTION);
        ui->btn_create->setEnabled(false);
        return;
    }

    InitPeer *thread_init = new InitPeer(this);
    thread_init->init(dir, os);
    thread_init->startWork();
    connect(thread_init, &InitPeer::outputReceived, [dir, ram, cpu, this](system_call_wrapper_error_t res){
       this->init_completed(res, dir, ram, cpu);
    });
}

//for peers, empty if that peer dir exists
QString DlgCreatePeer::create_dir(const QString &name){
    QString new_dir = "";
    QDir current_local_dir;
    QStringList stdDirList = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    QStringList::iterator stdDir = stdDirList.begin();
    if(stdDir == stdDirList.end())
      current_local_dir.setCurrent("/");
    else
      current_local_dir.setCurrent(*stdDir);
    current_local_dir.cd("Subutai-peers");
    if(!current_local_dir.mkdir(name))
        return new_dir;
    current_local_dir.cd(name);
    return current_local_dir.absolutePath();
}

void DlgCreatePeer::init_completed(system_call_wrapper_error_t res, QString dir, QString ram, QString cpu){
    ui->btn_create->setEnabled(true);
    if(res != SCWE_SUCCESS){
        CNotificationObserver::Instance()->Error("Coudn't create peer, sorry. Check if vagrant is installed correctly", DlgNotification::N_NO_ACTION);
        return;
    }
    CNotificationObserver::Instance()->Info("Initialization completed. Installing peer... Don't close terminal until instalation is compeleted", DlgNotification::N_NO_ACTION);
    res = CSystemCallWrapper::run_vagrant_up_in_terminal(dir, ram, cpu);
    if(res != SCWE_SUCCESS){
        CNotificationObserver::Instance()->Error("Coudn't start  peer, sorry", DlgNotification::N_NO_ACTION);
    }
}