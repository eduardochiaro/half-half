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

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var showSecondsToggle = clayConfig.getItemByMessageKey('SHOW_SECONDS');
    toggleBackground.call(showSecondsToggle);
    showSecondsToggle.on('change', toggleBackground);

    var textColorOverrideToggle = clayConfig.getItemByMessageKey('TEXT_COLOR_OVERRIDE');
    toggleTextColorOverride.call(textColorOverrideToggle);
    textColorOverrideToggle.on('change', toggleTextColorOverride);
  });
};