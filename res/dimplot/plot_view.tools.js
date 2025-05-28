// ManiVault invokes this function to set the plot data,
// when emitting qt_js_setDataInJS from the communication object
// The connection is established in qwebchannel.tools.js
function drawChart(d) {
    log("TaxonomyViewJS: taxonomy_view.tools.js: draw chart")

    rangeMin = parseFloat(d[0]);
    rangeMax = parseFloat(d[1]);
    log(d)
    log(rangeMin)
    log(rangeMax)
    setRangeMin(rangeMin);
    setRangeMax(rangeMax);
}

function setHeaderOptions(d)
{
    log("TaxonomyViewJS: taxonomy_view.tools.js: setHeaderOptions")
    
}
