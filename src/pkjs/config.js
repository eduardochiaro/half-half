module.exports = [
  {
    "type": "heading",
    "defaultValue": "Half/Half Settings"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Color Options"
      },
      {
        "type": "text",
        "defaultValue": "<img src=\"https://raw.githubusercontent.com/eduardochiaro/half-half/master/assets/basalt_1.png\" style=\"width: 144px; height: 168px; margin: 10px auto; display: block;\" />",
        "capabilities": ["PLATFORM_BASALT"],
      },
      {
        "type": "color",
        "messageKey": "SECONDARY_COLOR",
        "defaultValue": "007DCE",
        "label": "Upper Color",
        "sunlight": true,
        "allowGray": true
      },
      {
        "type": "color",
        "messageKey": "PRIMARY_COLOR",
        "defaultValue": "FFFFFF",
        "label": "Lower Color",
        "sunlight": true,
        "allowGray": true
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Display Options"
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_SECONDS",
        "label": "Show Seconds",
        "defaultValue": true,
      },
      {
        "type": "toggle",
        "messageKey": "BATTERY_SAVE_SECONDS",
        "label": "Battery Saving Mode",
        "description": "Automatically hide seconds after 10 seconds of inactivity to save battery. Seconds will reappear when you use the watch or move your wrist.",
        "defaultValue": false,
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
