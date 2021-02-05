/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "bt_shim_scanner"

#include "le_scanning_manager.h"

#include <base/bind.h>
#include <base/threading/thread.h>
#include <hardware/bluetooth.h>
#include <stdio.h>
#include <unordered_set>

#include "btif_common.h"
#include "gd/hci/address.h"
#include "gd/hci/le_scanning_manager.h"
#include "main/shim/entry.h"

class BleScannerInterfaceImpl : public BleScannerInterface,
                                public bluetooth::hci::ScanningCallback {
 public:
  ~BleScannerInterfaceImpl() override{};

  void Init() {
    bluetooth::shim::GetScanning()->RegisterScanningCallback(this);
  }

  /** Registers a scanner with the stack */
  void RegisterScanner(const bluetooth::Uuid& uuid, RegisterCallback) {
    LOG(INFO) << __func__ << " in shim layer";
    auto app_uuid = bluetooth::hci::Uuid::From128BitBE(uuid.To128BitBE());
    bluetooth::shim::GetScanning()->RegisterScanner(app_uuid);
  }

  /** Unregister a scanner from the stack */
  void Unregister(int scanner_id) {
    LOG(INFO) << __func__ << " in shim layer, scanner_id:" << scanner_id;
    bluetooth::shim::GetScanning()->Unregister(scanner_id);
  }

  /** Start or stop LE device scanning */
  void Scan(bool start) {
    LOG(INFO) << __func__ << " in shim layer";
    bluetooth::shim::GetScanning()->Scan(start);
  }

  /** Setup scan filter params */
  void ScanFilterParamSetup(
      uint8_t client_if, uint8_t action, uint8_t filter_index,
      std::unique_ptr<btgatt_filt_param_setup_t> filt_param,
      FilterParamSetupCallback cb) {
    LOG(INFO) << __func__ << " in shim layer";

    auto apcf_action = static_cast<bluetooth::hci::ApcfAction>(action);
    bluetooth::hci::AdvertisingFilterParameter advertising_filter_parameter;

    if (filt_param != nullptr) {
      if (filt_param && filt_param->dely_mode == 1) {
        // TODO refactor BTM_BleTrackAdvertiser
      }
      advertising_filter_parameter.feature_selection = filt_param->feat_seln;
      advertising_filter_parameter.list_logic_type =
          filt_param->list_logic_type;
      advertising_filter_parameter.filter_logic_type =
          filt_param->filt_logic_type;
      advertising_filter_parameter.rssi_high_thresh =
          filt_param->rssi_high_thres;
      advertising_filter_parameter.delivery_mode =
          static_cast<bluetooth::hci::DeliveryMode>(filt_param->dely_mode);
      if (filt_param && filt_param->dely_mode == 1) {
        advertising_filter_parameter.onfound_timeout =
            filt_param->found_timeout;
        advertising_filter_parameter.onfound_timeout_cnt =
            filt_param->found_timeout_cnt;
        advertising_filter_parameter.rssi_low_thres =
            filt_param->rssi_low_thres;
        advertising_filter_parameter.onlost_timeout = filt_param->lost_timeout;
        advertising_filter_parameter.num_of_tracking_entries =
            filt_param->num_of_tracking_entries;
      }
    }

    bluetooth::shim::GetScanning()->ScanFilterParameterSetup(
        apcf_action, filter_index, advertising_filter_parameter);
    // TODO refactor callback mechanism
    do_in_jni_thread(FROM_HERE, base::Bind(cb, 0, 0, 0));
  }

  /** Configure a scan filter condition  */
  void ScanFilterAdd(int filter_index, std::vector<ApcfCommand> filters,
                     FilterConfigCallback cb) {
    LOG(INFO) << __func__ << " in shim layer";
    std::vector<bluetooth::hci::AdvertisingPacketContentFilterCommand>
        new_filters = {};
    for (size_t i = 0; i < filters.size(); i++) {
      bluetooth::hci::AdvertisingPacketContentFilterCommand command{};
      if (!parse_filter_command(command, filters[i])) {
        LOG_ERROR("invalid apcf command");
        return;
      }
      new_filters.push_back(command);
    }
    bluetooth::shim::GetScanning()->ScanFilterAdd(filter_index, new_filters);
    do_in_jni_thread(FROM_HERE, base::Bind(cb, 0, 0, 0, 0));
  }

  /** Clear all scan filter conditions for specific filter index*/
  void ScanFilterClear(int filter_index, FilterConfigCallback cb) {
    LOG(INFO) << __func__ << " in shim layer";
    // This function doesn't used in java layer
  }

  /** Enable / disable scan filter feature*/
  void ScanFilterEnable(bool enable, EnableCallback cb) {
    LOG(INFO) << __func__ << " in shim layer";
    bluetooth::shim::GetScanning()->ScanFilterEnable(enable);

    uint8_t action = enable ? 1 : 0;
    do_in_jni_thread(FROM_HERE, base::Bind(cb, action, 0));
  }


  /** Sets the LE scan interval and window in units of N*0.625 msec */
  void SetScanParameters(int scan_phy, std::vector<uint32_t> scan_interval,
                         std::vector<uint32_t> scan_window,
                         Callback cb) {
    LOG(INFO) << __func__ << " in shim layer";
    // use active scan
    auto scan_type = static_cast<bluetooth::hci::LeScanType>(0x01);
    bluetooth::shim::GetScanning()->SetScanParameters(scan_type, scan_interval[0], scan_window[0]);
    do_in_jni_thread(FROM_HERE, base::Bind(cb, 0));
  }

  /* Configure the batchscan storage */
  void BatchscanConfigStorage(int client_if, int batch_scan_full_max,
                              int batch_scan_trunc_max,
                              int batch_scan_notify_threshold, Callback cb) {
    LOG(INFO) << __func__ << " in shim layer";
  }

  /* Enable batchscan */
  virtual void BatchscanEnable(int scan_mode, int scan_interval,
                               int scan_window, int addr_type, int discard_rule,
                               Callback cb) {
    LOG(INFO) << __func__ << " in shim layer";
  }

  /* Disable batchscan */
  virtual void BatchscanDisable(Callback cb) {
    LOG(INFO) << __func__ << " in shim layer";
  }

  /* Read out batchscan reports */
  void BatchscanReadReports(int client_if, int scan_mode) {
    LOG(INFO) << __func__ << " in shim layer";
  }

  void StartSync(uint8_t sid, RawAddress address, uint16_t skip,
                 uint16_t timeout, StartSyncCb start_cb, SyncReportCb report_cb,
                 SyncLostCb lost_cb) {
    LOG(INFO) << __func__ << " in shim layer";
    // This function doesn't implement in the old stack
  }

  void StopSync(uint16_t handle) {
    LOG(INFO) << __func__ << " in shim layer";
    // This function doesn't implement in the old stack
  }

  void RegisterCallbacks(ScanningCallbacks* callbacks) {
    LOG(INFO) << __func__ << " in shim layer";
    scanning_callbacks_ = callbacks;
  }

  void OnScannerRegistered(const bluetooth::hci::Uuid app_uuid,
                           bluetooth::hci::ScannerId scanner_id,
                           ScanningStatus status) {
    auto uuid = bluetooth::Uuid::From128BitBE(app_uuid.To128BitBE());
    do_in_jni_thread(FROM_HERE,
                     base::Bind(&ScanningCallbacks::OnScannerRegistered,
                                base::Unretained(scanning_callbacks_), uuid,
                                scanner_id, status));
  }

  void OnScanResult(uint16_t event_type, uint8_t address_type,
                    bluetooth::hci::Address address, uint8_t primary_phy,
                    uint8_t secondary_phy, uint8_t advertising_sid,
                    int8_t tx_power, int8_t rssi,
                    uint16_t periodic_advertising_interval,
                    std::vector<uint8_t> advertising_data) {
    RawAddress raw_address;
    RawAddress::FromString(address.ToString(), raw_address);

    do_in_jni_thread(
        FROM_HERE,
        base::BindOnce(&ScanningCallbacks::OnScanResult,
                       base::Unretained(scanning_callbacks_), event_type,
                       address_type, raw_address, primary_phy, secondary_phy,
                       advertising_sid, tx_power, rssi,
                       periodic_advertising_interval, advertising_data));
  }

  void OnTrackAdvFoundLost() {}

  void OnBatchScanReports(int client_if, int status, int report_format,
                          int num_records, std::vector<uint8_t> data) {}
  void OnTimeout() {}

  void OnFilterEnable(bluetooth::hci::Enable enable, uint8_t status){};

  void OnFilterParamSetup(uint8_t available_spaces,
                          bluetooth::hci::ApcfAction action, uint8_t status){};

  void OnFilterConfigCallback(bluetooth::hci::ApcfFilterType filter_type,
                              uint8_t available_spaces,
                              bluetooth::hci::ApcfAction action,
                              uint8_t status){};

  ScanningCallbacks* scanning_callbacks_;

  void CancelCreateSync(uint8_t sid, RawAddress address) override {}
  void TransferSync(RawAddress address, uint16_t service_data,
                         uint16_t sync_handle, SyncTransferCb cb) override {}

  void TransferSetInfo(RawAddress address, uint16_t service_data,
                         uint8_t adv_handle, SyncTransferCb cb) override {}
  void SyncTxParameters(RawAddress address, uint8_t mode, uint16_t skip,
                         uint16_t timeout, StartSyncCb cb) override {}

 private:
  bool parse_filter_command(
      bluetooth::hci::AdvertisingPacketContentFilterCommand&
          advertising_packet_content_filter_command,
      ApcfCommand apcf_command) {
    advertising_packet_content_filter_command.filter_type =
        static_cast<bluetooth::hci::ApcfFilterType>(apcf_command.type);
    bluetooth::hci::Address address;
    bluetooth::hci::Address::FromString(apcf_command.address.ToString(),
                                        address);
    advertising_packet_content_filter_command.address = address;
    advertising_packet_content_filter_command.application_address_type =
        static_cast<bluetooth::hci::ApcfApplicationAddressType>(
            apcf_command.addr_type);

    if (!apcf_command.uuid.IsEmpty()) {
      uint8_t uuid_len = apcf_command.uuid.GetShortestRepresentationSize();
      switch (uuid_len) {
        case bluetooth::Uuid::kNumBytes16: {
          advertising_packet_content_filter_command.uuid =
              bluetooth::hci::Uuid::From16Bit(apcf_command.uuid.As16Bit());
        } break;
        case bluetooth::Uuid::kNumBytes32: {
          advertising_packet_content_filter_command.uuid =
              bluetooth::hci::Uuid::From32Bit(apcf_command.uuid.As32Bit());
        } break;
        case bluetooth::Uuid::kNumBytes128: {
          advertising_packet_content_filter_command.uuid =
              bluetooth::hci::Uuid::From128BitBE(
                  apcf_command.uuid.To128BitBE());
        } break;
        default:
          LOG_WARN("illegal UUID length %d", (uint16_t)uuid_len);
          return false;
      }
    }

    if (!apcf_command.uuid_mask.IsEmpty()) {
      uint8_t uuid_len = apcf_command.uuid.GetShortestRepresentationSize();
      switch (uuid_len) {
        case bluetooth::Uuid::kNumBytes16: {
          advertising_packet_content_filter_command.uuid_mask =
              bluetooth::hci::Uuid::From16Bit(apcf_command.uuid_mask.As16Bit());
        } break;
        case bluetooth::Uuid::kNumBytes32: {
          advertising_packet_content_filter_command.uuid_mask =
              bluetooth::hci::Uuid::From32Bit(apcf_command.uuid_mask.As32Bit());
        } break;
        case bluetooth::Uuid::kNumBytes128: {
          advertising_packet_content_filter_command.uuid_mask =
              bluetooth::hci::Uuid::From128BitBE(
                  apcf_command.uuid_mask.To128BitBE());
        } break;
        default:
          LOG_WARN("illegal UUID length %d", (uint16_t)uuid_len);
          return false;
      }
    }

    advertising_packet_content_filter_command.name.assign(
        apcf_command.name.begin(), apcf_command.name.end());
    advertising_packet_content_filter_command.company = apcf_command.company;
    advertising_packet_content_filter_command.company_mask =
        apcf_command.company_mask;
    advertising_packet_content_filter_command.data.assign(
        apcf_command.data.begin(), apcf_command.data.end());
    advertising_packet_content_filter_command.data_mask.assign(
        apcf_command.data_mask.begin(), apcf_command.data_mask.end());
    return true;
  }
};

BleScannerInterfaceImpl* bt_le_scanner_instance = nullptr;

BleScannerInterface* bluetooth::shim::get_ble_scanner_instance() {
  if (bt_le_scanner_instance == nullptr) {
    bt_le_scanner_instance = new BleScannerInterfaceImpl();
  }
  return bt_le_scanner_instance;
}

void bluetooth::shim::init_scanning_manager() {
  bt_le_scanner_instance->Init();
}
