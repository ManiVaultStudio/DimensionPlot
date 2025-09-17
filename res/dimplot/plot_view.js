function exportSvgToPngViaQtOrBrowser(svgNode, { filename = 'chart.png', scale = 2 } = {}) {
  const width  = svgNode.width.baseVal.value || svgNode.getBoundingClientRect().width;
  const height = svgNode.height.baseVal.value || svgNode.getBoundingClientRect().height;

  const cloned = svgNode.cloneNode(true);
  cloned.setAttribute('xmlns','http://www.w3.org/2000/svg');
  cloned.setAttribute('xmlns:xlink','http://www.w3.org/1999/xlink');

  const svgData = new XMLSerializer().serializeToString(cloned);
  const blob = new Blob([svgData], { type: 'image/svg+xml;charset=utf-8' });
  const url = URL.createObjectURL(blob);

  const img = new Image();
  img.onload = () => {
    const canvas = document.createElement('canvas');
    canvas.width = Math.max(1, Math.floor(width  * scale));
    canvas.height= Math.max(1, Math.floor(height * scale));
    const ctx = canvas.getContext('2d');

    // White background; remove if you want transparency
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.drawImage(img, 0, 0, canvas.width, canvas.height);

    // Prefer Qt save path if available
    {
      const dataUrl = canvas.toDataURL('image/png'); // data:image/png;base64,....
      // Let Qt show a Save dialog and write the file
      exportSvgToPng(dataUrl, filename);
      URL.revokeObjectURL(url);
      return;
    }

    // Fallback: browser download
    canvas.toBlob((pngBlob) => {
      const a = document.createElement('a');
      a.href = URL.createObjectURL(pngBlob);
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      a.remove();
      URL.revokeObjectURL(a.href);
      URL.revokeObjectURL(url);
    }, 'image/png');
  };
  img.onerror = (e) => { console.error('PNG export failed', e); URL.revokeObjectURL(url); };
  img.src = url;
}

document.addEventListener('DOMContentLoaded', () => {
  const btn = document.getElementById('btnDownload');
  if (btn) {
    btn.addEventListener('click', () => {
      const svg = document.querySelector('svg');
      const scale = Math.max(0.5, parseFloat(document.getElementById('downloadScale')?.value || '2')) || 2;
      const filename = (document.getElementById('downloadName')?.value || 'chart.png').trim();
      exportSvgToPngViaQtOrBrowser(svg, { filename, scale });
    });
  }
});

function plotData(jsonDoc) {
    const svg = d3.select("svg");
    svg.selectAll("*").remove();

    const { title, values, categories } = jsonDoc;

    // Convert category array to a map: { name: [indices] }
    const categoryMap = {};
    const categoryColorMap = {};

    for (const { name, indices, color } of categories) {
      categoryMap[name] = indices;
      categoryColorMap[name] = color;
    }

    // Flatten data into array of { value, category }
    const flatData = [];
    for (const [cat, indices] of Object.entries(categoryMap)) {
        for (const i of indices) {
            flatData.push({ value: values[i], category: cat });
        }
    }

    const grouped = d3.group(flatData, d => d.category);
    const categoryNames = Array.from(grouped.keys());

    // Layout setup
    const margin = { top: 50, right: 30, bottom: 70, left: 50 };
    const width = +svg.attr("width") - margin.left - margin.right;
    const height = +svg.attr("height") - margin.top - margin.bottom;
    const g = svg.append("g").attr("transform", `translate(${margin.left},${margin.top})`);

    // Title
    svg.append("text")
        .attr("x", +svg.attr("width") / 2)
        .attr("y", margin.top / 2)
        .attr("text-anchor", "middle")
        .attr("font-size", "18px")
        .attr("font-weight", "bold")
        .text(title);

    // Scales
    const x = d3.scaleBand()
        .domain(categoryNames)
        .range([0, width])
        .padding(0.4);

    const allValues = flatData.map(d => d.value);
    const padding = (d3.max(allValues) - d3.min(allValues)) * 0.1;

    const y = d3.scaleLinear()
        .domain([d3.min(allValues) - padding, d3.max(allValues) + padding])
        .range([height, 0]);  // Flip: higher value = higher up

    const color = d3.scaleOrdinal()
        .domain(categoryNames)
        .range(d3.schemeCategory10);

    // Axes
    g.append("g")
        .attr("transform", `translate(0,${height})`)
        .call(d3.axisBottom(x))
        .selectAll("text")
        .attr("transform", "rotate(-45)")
        .style("text-anchor", "end")
        .style("font-size", "14px");

    g.append("g").call(d3.axisLeft(y));

    const boxWidth = x.bandwidth();

    // Draw boxplots
    for (const [category, group] of grouped) {
        const valuesSorted = group.map(d => d.value).sort(d3.ascending);
        const q1 = d3.quantileSorted(valuesSorted, 0.25);
        const median = d3.quantileSorted(valuesSorted, 0.5);
        const q3 = d3.quantileSorted(valuesSorted, 0.75);
        const min = d3.min(valuesSorted);
        const max = d3.max(valuesSorted);

        const xPos = x(category);
        const mid = xPos + boxWidth / 2;

        // Ensure positive box height
        const boxTop = y(q3);
        const boxBottom = y(q1);
        const boxHeight = boxBottom - boxTop;

        // Compute color for this category
        const strokeColor = categoryColorMap[category];

        // Box (outline only)
        g.append("rect")
          .attr("x", xPos)
          .attr("y", boxTop)
          .attr("width", boxWidth)
          .attr("height", boxHeight)
          .attr("fill", "none")
          .attr("stroke", strokeColor)
          .attr("stroke-width", 1.5);

        // Median line
        g.append("line")
          .attr("x1", xPos)
          .attr("x2", xPos + boxWidth)
          .attr("y1", y(median))
          .attr("y2", y(median))
          .attr("stroke", strokeColor)
          .attr("stroke-width", 2);

        // Whiskers
        g.append("line")
          .attr("x1", mid).attr("x2", mid)
          .attr("y1", y(min)).attr("y2", y(q1))
          .attr("stroke", strokeColor);

        g.append("line")
          .attr("x1", mid).attr("x2", mid)
          .attr("y1", y(q3)).attr("y2", y(max))
          .attr("stroke", strokeColor);

        // Whisker caps
        g.append("line")
          .attr("x1", mid - boxWidth / 4).attr("x2", mid + boxWidth / 4)
          .attr("y1", y(min)).attr("y2", y(min))
          .attr("stroke", strokeColor);

        g.append("line")
          .attr("x1", mid - boxWidth / 4).attr("x2", mid + boxWidth / 4)
          .attr("y1", y(max)).attr("y2", y(max))
          .attr("stroke", strokeColor);

        const jitterWidth = boxWidth * 0.3;
        const jitterOffset = (boxWidth - jitterWidth) / 2;

        g.selectAll(null)
          .data(group)
          .enter()
          .append("circle")
          .attr("cx", () => xPos + jitterOffset + Math.random() * jitterWidth)
          .attr("cy", d => y(d.value))
          .attr("r", 2)
          .attr("fill", strokeColor)
          .attr("opacity", 0.5);
    }
}
