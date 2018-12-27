
#include "JadeManager.hh"

using namespace std::chrono_literals;

//+++++++++++++++++++++++++++++++++++++++++
//AltelManager.hh
class AltelManager : public JadeManager {
  public:
  AltelManager(const JadeOption& opt);
  ~AltelManager() override{};
  JadeOption Post(const std::string& url, const JadeOption& opt) override;
  void Init() override;
  void StartDataTaking() override;
  void StopDataTaking() override;
  std::string SendCommand(const std::string& cmd, const std::string& para) override;
  void DeviceControl(const std::string &cmd) override;

  JadeRegCtrlSP m_ctrl;
  JadeReaderSP m_rd;
  JadeFilterSP m_flt;
  JadeWriterSP m_wrt;
  JadeMonitorSP m_mnt;
  JadeOption m_opt;
};

//+++++++++++++++++++++++++++++++++++++++++
//AltelManager.cc
namespace {
auto _test_index_ = JadeUtils::SetTypeIndex(std::type_index(typeid(AltelManager)));
auto _test_ = JadeFactory<JadeManager>::Register<AltelManager, const JadeOption&>(typeid(AltelManager));
}

AltelManager::AltelManager(const JadeOption& opt)
    : JadeManager(opt)
    , m_opt(opt)
{
}

JadeOption AltelManager::Post(const std::string& url, const JadeOption& opt)
{
  static const std::string url_base_class("/JadeManager/");
  if (!url.compare(0, url_base_class.size(), url_base_class)) {
    return JadeManager::Post(url.substr(url_base_class.size() - 1), opt);
  }
  return JadePost::Post(url, opt);
}

void AltelManager::Init()
{
  MakeComponent();

  m_ctrl = GetRegCtrl();
  m_rd = GetReader();
  m_flt = GetFilter();
  m_wrt = GetWriter();
  m_mnt = GetMonitor();

  //hardware specific
  m_ctrl->Open();

  std::string chip_select = m_opt.GetStringValue("CHIP_SELECT");
  m_ctrl->SendCommand(chip_select);
  std::cout << chip_select << std::endl;
  std::string delay = m_opt.GetStringValue("DELAY");
  m_ctrl->SendCommand(delay);
  std::cout << delay << std::endl;
  m_ctrl->SendCommand("SET");
  std::cout << "Read back chip address setting: " << unsigned(m_ctrl->ReadByte(0x08)) << std::endl;
  std::cout << "Read back delay setting: " << unsigned(m_ctrl->ReadByte(0x05)) << std::endl;
  std::this_thread::sleep_for(200ms);
  m_ctrl->Close();
}

void AltelManager::DeviceControl(const std::string &cmd)
{
  m_ctrl->SendCommand(cmd);
}

void AltelManager::StartDataTaking()
{
  m_rd->Open();
  m_ctrl->Open();
  m_wrt->Open();
  m_flt->Reset();
  m_mnt->Reset();

  //hardware specific
  m_ctrl->SendCommand("STOP");
  std::this_thread::sleep_for(1s);
  m_ctrl->SendCommand("START");

  //Can threads start before the device's startup?
  StartThread();
}

void AltelManager::StopDataTaking()
{
  //Can threads terminate before the device's shutdown?
  StopThread();
  
  //hardware specific
  m_ctrl->SendCommand("STOP");
  m_rd->Close();
  m_ctrl->Close();
  m_wrt->Close();
}

std::string AltelManager::SendCommand(const std::string& cmd, const std::string& para)
{
  if (cmd == "REOPEN_CTRL_PATH") {
    //TODO: what will happen if return a nullptr?
    std::string ctrl_name = JadeUtils::NameDemangle(typeid(*(GetRegCtrl())).name());
    if (ctrl_name != "AltelRegCtrl" && ctrl_name != "Altel1RegCtrl") {
      //NOTE: prevent this command be used when other RegCtrl is loaded.
      std::cerr << "ERROR, type mismatch";
      throw;
    }
    GetRegCtrl()->Close();
    //NOTE: alter the path of a ctrl deivce is innormal, support by specific RegCtrl implement.
    //generic Post inferface is used for specific method.
    Post(JadeUtils::FormatString("/%s/set_path", ctrl_name.c_str()),
        JadeUtils::FormatString("{\"PATH\":\"%s\"}", para.c_str()));
    GetRegCtrl()->Open();
    return "";
  }

  if (cmd == "REOPEN_DATA_IN_PATH") {
    std::string reader_name = JadeUtils::NameDemangle(typeid(*GetReader()).name());
    if (reader_name != "AltelReader" && reader_name != "Altel1Reader" && reader_name != "Altel2Reader") {
      std::cerr << "ERROR, type mismatch";
      throw;
    }
    GetReader()->Close();
    Post(JadeUtils::FormatString("/%s/set_path", reader_name.c_str()),
        JadeUtils::FormatString("{\"PATH\":\"%s\"}", para.c_str()));
    GetReader()->Open();
    return "";
  }

  if (cmd == "REOPEN_DATA_OUT_PATH") {
    std::string writer_name = JadeUtils::NameDemangle(typeid(*GetWriter()).name());
    //NOTE: optional test the writer_name
    GetWriter()->Close();
    Post(JadeUtils::FormatString("/%s/set_path", writer_name.c_str()),
        JadeUtils::FormatString("{\"PATH\":\"%s\"}", para.c_str()));
    GetWriter()->Open();
    return "";
  }

  if (cmd == "CTRL_CHIPA_SELECT") {
    //NOTE: can be POST
    GetRegCtrl()->SendCommand(para);
    GetRegCtrl()->SendCommand("SET");
    std::this_thread::sleep_for(200ms);
    GetRegCtrl()->Close();
    GetRegCtrl()->Open();
    return "";
  }

  if (cmd == "MON_ADC_THREASHOLD") {
    //NOTE: Monitors can be very diverse, we can not assumme the inferface in Manager.
    std::string mon_name = JadeUtils::NameDemangle(typeid((*GetMonitor())).name());
    Post(JadeUtils::FormatString("/%s/set_adc_threshold", mon_name.c_str()),
        //TODO: is THREASHOLD a string?
        JadeUtils::FormatString("{\"ADC_THRESHOLD\":\"%s\"}", para.c_str()));
    return "";
  }

  if (cmd == "MON_ROW") {
    std::string mon_name = JadeUtils::NameDemangle(typeid(*GetMonitor()).name());
    Post(JadeUtils::FormatString("/%s/set_row", mon_name.c_str()),
        JadeUtils::FormatString("{\"ROW\":%u}", std::stoul(para)));
    return "";
  }

  if (cmd == "MON_COLUMN") {
    std::string mon_name = JadeUtils::NameDemangle(typeid(*GetMonitor()).name());
    Post(JadeUtils::FormatString("/%s/set_column", mon_name.c_str()),
        JadeUtils::FormatString("{\"COLUMN\":%u}", std::stoul(para)));
    return "";
  }

  if (cmd == "MON_PRINT_EVNET_N") {
    std::string mon_name = JadeUtils::NameDemangle(typeid(*GetMonitor()).name());
    Post(JadeUtils::FormatString("/%s/set_print_event_n", mon_name.c_str()),
        JadeUtils::FormatString("{\"PRINT_EVENT_N\":%u}", std::stoul(para)));
    return "";
  }

  std::cout<<"AltelManager:: no match command processing"<<std::endl;
  return "";
}
