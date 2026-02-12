module.exports = function(minified) {
  var clayConfig = this;
  var _ = minified._;
  var $ = minified.$;
  var HTML = minified.HTML;

  function toggleBackground() {
    if (this.get()) {
      clayConfig.getItemByMessageKey('BATTERY_SAVE_SECONDS').enable();
    } else {
      clayConfig.getItemByMessageKey('BATTERY_SAVE_SECONDS').disable();
    }
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var coolStuffToggle = clayConfig.getItemByMessageKey('SHOW_SECONDS');
    toggleBackground.call(coolStuffToggle);
    coolStuffToggle.on('change', toggleBackground);
  });
};