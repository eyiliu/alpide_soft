
#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Logger.hh"

#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <bitset>

//////////////////bit order
#ifdef _WIN32
#include <crtdefs.h>
#define BE16TOH ntohs
#define BE32TOH ntohl
#define BE64TOH ntohll
#define LE16TOH
#define LE32TOH
#define LE64TOH
#else

#ifdef __APPLE__
#include <machine/endian.h>
#if __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN
#define BE16TOH NTOHS
#define BE32TOH NTOHL
#define BE64TOH NTOHLL
#define LE16TOH
#define LE32TOH
#define LE64TOH
#endif
#else

#include <endian.h>
#define BE16TOH be16toh
#define BE32TOH be32toh
#define BE64TOH be64toh
#define LE16TOH le16toh
#define LE32TOH le32toh
#define LE64TOH le64toh
#endif
#endif
///////////////////////

class ATLASFEI4Interpreter {
public:
  //-----------------
  // Data Header (dh)
  //-----------------
  static const uint32_t dh_wrd = 0x00E90000;
  static const uint32_t dh_msk = 0xFFFF0000;
  static const uint32_t dh_flag_msk = 0x00008000;

  //-----------------
  // Data Record (dr)
  //-----------------
  static const uint32_t dr_col_msk = 0x00FE0000;
  static const uint32_t dr_row_msk = 0x0001FF00;
  static const uint32_t dr_tot1_msk = 0x000000F0;
  static const uint32_t dr_tot2_msk = 0x0000000F;

  // limits of FE-size
  static const uint32_t rd_min_col = 1;
  static const uint32_t rd_max_col = 80;
  static const uint32_t rd_min_row = 1;
  static const uint32_t rd_max_row = 336;

  // the limits shifted for easy verification with a dr
  static const uint32_t dr_min_col = rd_min_col << 17;
  static const uint32_t dr_max_col = rd_max_col << 17;
  static const uint32_t dr_min_row = rd_min_row << 8;
  static const uint32_t dr_max_row = rd_max_row << 8;

  //-----------------
  // Trigger Data (tr)
  //-----------------
  static const uint32_t tr_wrd_hdr_v10 = 0x00FFFF00;
  static const uint32_t tr_wrd_hdr_msk_v10 = 0xFFFFFF00;
  static const uint32_t tr_wrd_hdr = 0x00F80000; // tolerant to 1-bit flips and
  // not equal to control/comma
  // symbols
  static const uint32_t tr_wrd_hdr_msk = 0xFFFF0000;

  static const uint32_t tr_no_31_24_msk = 0x000000FF;
  static const uint32_t tr_no_23_0_msk = 0x00FFFFFF;

  static const uint32_t tr_data_msk = 0x0000FF00; // trigger error + trigger mode
  static const uint32_t tr_mode_msk = 0x0000E000; // trigger mode
  static const uint32_t tr_err_msk = 0x00001F00;  // error code: bit 0: wrong

  static bool is_dh(uint32_t X) { return (dh_msk & X) == dh_wrd; }  
  static bool is_dr(uint32_t X) {
    return (((dr_col_msk & X) <= dr_max_col) &&
	    ((dr_col_msk & X) >= dr_min_col) &&
	    ((dr_row_msk & X) <= dr_max_row) &&
	    ((dr_row_msk & X) >= dr_min_row));
  }
  static uint32_t get_dr_col1(uint32_t X) { return (dr_col_msk & X) >> 17; }
  static uint32_t get_dr_row1(uint32_t X) { return (dr_row_msk & X) >> 8; }
  static uint32_t get_dr_tot1(uint32_t X) { return (dr_tot1_msk & X) >> 4; }
  static uint32_t get_dr_col2(uint32_t X) { return (dr_col_msk & X) >> 17; }
  static uint32_t get_dr_row2(uint32_t X) { return ((dr_row_msk & X) >> 8) + 1;}
  static uint32_t get_dr_tot2(uint32_t X) { return (dr_tot2_msk & X); }
};

class UsbpixrefRawEvent2LCEventConverter: public eudaq::LCEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf) const override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("USBPIXI4");
private:
  bool getHitData(uint32_t &Word, bool second_hit,
		  uint32_t &Col, uint32_t &Row, uint32_t &ToT) const;
  static const uint32_t CHIP_MIN_COL = 1;
  static const uint32_t CHIP_MAX_COL = 80;
  static const uint32_t CHIP_MIN_ROW = 1;
  static const uint32_t CHIP_MAX_ROW = 336;
  static const uint32_t CHIP_MAX_ROW_NORM = CHIP_MAX_ROW - CHIP_MIN_ROW;
  static const uint32_t CHIP_MAX_COL_NORM = CHIP_MAX_COL - CHIP_MIN_COL;
  uint32_t consecutive_lvl1 = 16;
  uint32_t first_sensor_id = 0;
  uint32_t chip_id_offset = 10;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::LCEventConverter>::
    Register<UsbpixrefRawEvent2LCEventConverter>(UsbpixrefRawEvent2LCEventConverter::m_id_factory);
}

bool UsbpixrefRawEvent2LCEventConverter::
Converting(eudaq::EventSPC d1, eudaq::LCEventSP d2, eudaq::ConfigurationSPC conf)const {
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  d2->parameters().setValue("EventType",2);
  lcio::LCCollectionVec * zsDataCollection = nullptr;
  auto p_col_names = d2->getCollectionNames();
  if(std::find(p_col_names->begin(), p_col_names->end(), "zsdata_apix") != p_col_names->end()){
    zsDataCollection = dynamic_cast<lcio::LCCollectionVec*>(d2->getCollection("zsdata_alpide"));
    if(!zsDataCollection)
      EUDAQ_THROW("dynamic_cast error: from lcio::LCCollection to lcio::LCCollectionVec");
  }
  else{
    zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    d2->addCollection(zsDataCollection, "zsdata_apix");
  }
  lcio::CellIDEncoder<lcio::TrackerDataImpl> zsDataEncoder("sensorID:7,sparsePixelType:5", zsDataCollection);
  auto block_n_list = ev->GetBlockNumList();
  for(auto &chip: block_n_list){
    auto rawblock = ev->GetBlock(chip);
    uint32_t* buffer = reinterpret_cast<uint32_t*>(rawblock.data());
    size_t buffer_size = rawblock.size()/sizeof(uint32_t);
    
    lcio::TrackerDataImpl *zsFrame = new lcio::TrackerDataImpl;
    zsDataEncoder["sensorID"] = chip + chip_id_offset + first_sensor_id;;
    zsDataEncoder["sparsePixelType"] = 2;//eutelescope::kEUTelGenericSparsePixel
    zsDataEncoder.setCellID(zsFrame);

    uint32_t ToT = 0;
    uint32_t Col = 0;
    uint32_t Row = 0;
    uint32_t lvl1 = 0;

    for(size_t i=0; i < buffer_size; i ++){
      uint32_t Word = LE32TOH(buffer[i]);
      if(ATLASFEI4Interpreter::is_dh(Word)){
  	lvl1++;
      }
      else{
  	if(lvl1==0){
  	  //std::cout<< "fei4 event might be invalid, skipped"<<std::endl;
  	  break; //hacking, TODO: otherwise levl1 - 1  = -1, not acceptable!
  	}
  	//First Hit
  	if(getHitData(Word, false, Col, Row, ToT)){
  	  zsFrame->chargeValues().push_back(335-Row);//x
  	  zsFrame->chargeValues().push_back(Col);//y
  	  zsFrame->chargeValues().push_back(ToT);//signal
  	  zsFrame->chargeValues().push_back(lvl1-1);//time
  	}
  	//Second Hit
  	if(getHitData(Word, true, Col, Row, ToT)){
  	  zsFrame->chargeValues().push_back(335-Row);
  	  zsFrame->chargeValues().push_back(Col);
  	  zsFrame->chargeValues().push_back(ToT);
  	  zsFrame->chargeValues().push_back(lvl1-1);
  	}
      }
    }
    zsDataCollection->push_back( zsFrame);
  }
}

bool UsbpixrefRawEvent2LCEventConverter::
getHitData(uint32_t &Word, bool second_hit,
	   uint32_t &Col, uint32_t &Row, uint32_t &ToT) const{
  //No data record
  if( !ATLASFEI4Interpreter::is_dr(Word)){
    return false;
  }
  uint32_t t_Col=0;
  uint32_t t_Row=0;
  uint32_t t_ToT=15;

  if(!second_hit){
    t_ToT = ATLASFEI4Interpreter::get_dr_tot1(Word);
    t_Col = ATLASFEI4Interpreter::get_dr_col1(Word);
    t_Row = ATLASFEI4Interpreter::get_dr_row1(Word);
  }
  else{
    t_ToT = ATLASFEI4Interpreter::get_dr_tot2(Word);
    t_Col = ATLASFEI4Interpreter::get_dr_col2(Word);
    t_Row = ATLASFEI4Interpreter::get_dr_row2(Word);
  }

  //translate FE-I4 ToT code into tot
  //tot_mode = 0
  if (t_ToT==14 || t_ToT==15)
    return false;
  ToT = t_ToT + 1;

  if(t_Row > CHIP_MAX_ROW || t_Row < CHIP_MIN_ROW){
    std::cout << "Invalid row: " << t_Row << std::endl;
    return false;
  }
  if(t_Col > CHIP_MAX_COL || t_Col < CHIP_MIN_COL){
    std::cout << "Invalid col: " << t_Col << std::endl;
    return false;
  }
  //Normalize Pixelpositions
  t_Col -= CHIP_MIN_COL;
  t_Row -= CHIP_MIN_ROW;
  Col = t_Col;
  Row = t_Row;
  return true;
}
