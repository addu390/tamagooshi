(function () {
  const id = new URLSearchParams(location.search).get("brand");
  if (!id || !/^[a-z0-9-]+$/.test(id)) return;
  document.write('<link rel="stylesheet" href="common/css/themes/' + id + '.css">');
  document.write('<script src="common/js/presets/' + id + '.js"><\/script>');
})();
