(function () {
  const id = new URLSearchParams(location.search).get("brand");
  if (!id || !/^[a-z0-9-]+$/.test(id)) return;
  document.write('<link rel="stylesheet" href="css/themes/' + id + '.css">');
  document.write('<script src="js/presets/' + id + '.js"><\/script>');
})();
