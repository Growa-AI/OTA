# ESP32 OTA Update Firmware

## Overview
Robust Over-The-Air (OTA) update mechanism for ESP32 devices, enabling seamless firmware updates via GitHub repository.

## Features
- Automatic firmware version checking
- Secure HTTPS downloads
- Multiple update attempt management
- Detailed error logging
- WiFi connection handling
- Cache-busting mechanisms

## Prerequisites
- ESP32 Development Board
- Arduino IDE
- WiFi Network
- GitHub Repository for Firmware Hosting

## Configuration

### WiFi Credentials
Update the following constants in the code:
```cpp
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
```

### Update URLs
Modify these URLs to point to your firmware files:
```cpp
const char* version_url = "https://raw.githubusercontent.com/YourUsername/YourRepo/main/firmware/version.txt";
const char* firmware_url = "https://raw.githubusercontent.com/YourUsername/YourRepo/main/firmware/firmware.ino.bin";
```

## Update Process
1. Create `version.txt` with new version number
2. Upload compiled `.bin` firmware to repository
3. Device automatically checks and downloads updates

## Error Handling
- Maximum 3 update attempts
- Logs detailed error messages
- Automatic rollback to previous firmware

## Libraries Required
- WiFi
- HTTPClient
- Update
- WiFiClientSecure

## Security Notes
- Uses insecure client (modify for production)
- Implement certificate verification in sensitive deployments

## Customization
Adjust these parameters as needed:
- `MAX_UPDATE_ATTEMPTS`
- Update interval
- Logging verbosity

## Troubleshooting
- Check serial monitor for debug messages
- Verify network connectivity
- Confirm firmware file accessibility

## Contributing
Contributions welcome. Please open issues or submit pull requests.
