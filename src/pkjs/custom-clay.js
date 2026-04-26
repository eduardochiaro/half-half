module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

  function toggleBackground() {
    var batterySaveSecondsToggle = clayConfig.getItemByMessageKey('BATTERY_SAVE_SECONDS');
    if (this.get()) {
      batterySaveSecondsToggle.enable();
    } else {
      batterySaveSecondsToggle.disable();
    }
  }

  function toggleTextColorOverride() {
    var textOverrideColor = clayConfig.getItemByMessageKey('TEXT_OVERRIDE_COLOR');
    if (this.get()) {
      textOverrideColor.enable();
    } else {
      textOverrideColor.disable();
    }
  }

  function RGBtoHex(rgb) {
    return '#' + ((1 << 24) + (rgb[0] << 16) + (rgb[1] << 8) + rgb[2]).toString(16).slice(1).toUpperCase();
  }

  function decimalToRGB(decimal) {
    var r = (decimal >> 16) & 255;
    var g = (decimal >> 8) & 255;
    var b = decimal & 255;
    return RGBtoHex([r, g, b]);
  }


  var PRESETS = [
    { name: 'Ocean',   upper: '007DCE', lower: 'FFFFFF', text: null },
    { name: 'Night',   upper: '000055', lower: 'FFAA00', text: null },
    { name: 'Fire',    upper: 'FF5500', lower: '000000', text: null },
    { name: 'Forest',  upper: '005500', lower: '00AA00', text: 'FFFFFF' },
    { name: 'Dusk',    upper: 'AA5500', lower: '000055', text: 'FFFFFF' },
    { name: 'Sunset',   upper: 'FFAA00', lower: '550000', text: null },
    { name: 'Lavender',upper: 'AA00AA', lower: 'FFFFFF', text: null },
    { name: 'Cotton Candy', upper: 'FF55AA', lower: 'FFAAFF', text: 'FFFFFF' },
    { name: 'Rose',    upper: 'FF0055', lower: 'FFFFFF', text: null },
    { name: 'Slate',   upper: '555555', lower: 'FFFFFF', text: null },
  ];

  function buildPresets() {
    var presetsItem = clayConfig.getItemById('COLOR_PRESETS');
    var primaryColorItem = clayConfig.getItemByMessageKey('PRIMARY_COLOR');
    var secondaryColorItem = clayConfig.getItemByMessageKey('SECONDARY_COLOR');
    var textOverrideColorItem = clayConfig.getItemByMessageKey('TEXT_OVERRIDE_COLOR');

    var html = '<div id="hh-presets" style="display:flex;flex-wrap:nowrap;overflow-x:auto;gap:6px;padding:4px 0;-webkit-overflow-scrolling:touch;">';
    PRESETS.forEach(function(p) {
      html += '<button type=\"button\" data-upper=\"' + p.upper + '\"' +
        ' data-lower=\"' + p.lower + '\"' +
        (p.text ? ' data-text=\"' + p.text + '\"' : '') +
        ' style="border:2px solid #ccc;border-radius:6px;padding:0;width:40px;cursor:pointer;overflow:hidden;background:none;">' +
        '<div style="height:20px;background:#' + p.upper + ';"></div>' +
        '<div style="height:20px;background:#' + p.lower + ';"></div>' +
        '<div style="font-size:14px;font-weight:bold;padding:2px 0;text-align:center;background:#' + p.upper + ';color:#' +  (p.text || p.lower) + ';">' + p.name + '</div>' +
        '</button>';
    });
    html += '</div>';

    presetsItem.set(html);

    document.querySelectorAll('#hh-presets button').forEach(function(btn) {
      btn.addEventListener('click', function() {
        secondaryColorItem.set(parseInt(btn.getAttribute('data-upper'), 16));
        primaryColorItem.set(parseInt(btn.getAttribute('data-lower'), 16));
        if (btn.hasAttribute('data-text')) {
          clayConfig.getItemByMessageKey('USE_TEXT_COLOR_OVERRIDE').set(true);
          textOverrideColorItem.set(parseInt(btn.getAttribute('data-text'), 16));
        } else {         
          clayConfig.getItemByMessageKey('USE_TEXT_COLOR_OVERRIDE').set(false);
        }
      });
    });
  }

  function togglePreview() {
    var preview = clayConfig.getItemById('PREVIEW');
    var primaryColor = decimalToRGB(clayConfig.getItemByMessageKey('PRIMARY_COLOR').get());
    var secondaryColor = decimalToRGB(clayConfig.getItemByMessageKey('SECONDARY_COLOR').get());
    var textColorOverrideToggle = clayConfig.getItemByMessageKey('USE_TEXT_COLOR_OVERRIDE');
    var textOverrideColor = decimalToRGB(clayConfig.getItemByMessageKey('TEXT_OVERRIDE_COLOR').get());

    var topTextColor = textColorOverrideToggle.get() ? textOverrideColor : primaryColor;
    var bottomTextColor = textColorOverrideToggle.get() ? textOverrideColor : secondaryColor;

    var previewHTML = '' +
      '<div style="width:200px;height:228px;margin:0 auto;position:relative;font-family:\'Avenir Next Condensed\',\'Arial Narrow\',sans-serif;overflow:hidden;background:' + primaryColor + ';">' +
        '<div style="position:absolute;left:0;right:0;top:0;height:114px;background:' + secondaryColor + ';"></div>' +
        '<div style="position:absolute;left:0;right:0;top:114px;height:114px;background:' + primaryColor + ';"></div>' +

        '<div style="position:absolute;top:18px;left:0;right:0;text-align:center;font-size:60px;font-weight:700;line-height:1;color:' + topTextColor + ';">8</div>' +

        '<div style="position:absolute;left:0;right:0;top:90px;height:28px;font-size:18px;font-weight:700;line-height:1;">' +
          '<span style="position:absolute;left:10px;top:0;color:' + topTextColor + ';">Steps</span>' +
          '<span style="position:absolute;right:10px;top:0;color:' + topTextColor + ';">Batt</span>' +
          '<span style="position:absolute;left:24px;top:35px;color:' + bottomTextColor + ';">0</span>' +
          '<span style="position:absolute;right:8px;top:35px;color:' + bottomTextColor + ';">80%</span>' +
        '</div>' +

        '<div style="font-size: 18px; line-height: 1.05; position: absolute; background-color: ' + secondaryColor + ';color:' + topTextColor + '; border-radius: 50%; width: 40px; height: 40px; display: flex; align-items: center; justify-content: center; top: 94px; left: 60px;font-weight: bold;">APR</div>' +
        '<div style="font-size: 18px; line-height: 1.05; position: absolute; background-color: ' + primaryColor + ';color:' + bottomTextColor + '; border-radius: 50%; width: 40px; height: 40px; display: flex; align-items: center; justify-content: center; top: 94px; left: 100px;font-weight: bold;">26</div>' +

        '<div style="position:absolute;left:0;right:0;top:138px;color:' + bottomTextColor + ';text-align:center;font-weight:700;">' +
          '<div style="font-size:60px;line-height:0.95;">34</div>' +
          '<div style="font-size:32px;line-height:0.88;">33</div>' +
        '</div>' +
      '</div>';

    preview.set(previewHTML);
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    buildPresets();

    var showSecondsToggle = clayConfig.getItemByMessageKey('SHOW_SECONDS');
    toggleBackground.call(showSecondsToggle);
    showSecondsToggle.on('change', toggleBackground);

    var textColorOverrideToggle = clayConfig.getItemByMessageKey('USE_TEXT_COLOR_OVERRIDE');
    toggleTextColorOverride.call(textColorOverrideToggle);
    textColorOverrideToggle.on('change', toggleTextColorOverride);

    var primaryColor = clayConfig.getItemByMessageKey('PRIMARY_COLOR');
    var secondaryColor = clayConfig.getItemByMessageKey('SECONDARY_COLOR');
    var textOverrideColor = clayConfig.getItemByMessageKey('TEXT_OVERRIDE_COLOR');

    togglePreview.call(primaryColor);
    textColorOverrideToggle.on('change', togglePreview);
    primaryColor.on('change', togglePreview);
    secondaryColor.on('change', togglePreview);
    textOverrideColor.on('change', togglePreview);
  });
};