/*
   webalizer - a web server log analysis program

   Copyright (c) 2016-2021, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     
   
   webalizer_highcharts.js
*/

///
/// @brief  Returns an array of X axis references to the position of the series data 
///         in the original data array.
///
/// Input data is received in sparse arrays, with data present only for existing data
/// points. This function returns an array of as many elements as there are data points 
/// in the series, each containing either `null` or the index of the element within the 
/// input data array. For example, if monthly usage series is populated only for 3 days 
/// in the input array (e.g. `4`, `5` and `28`), elements `3`, `4` and `27` of the returned 
/// array will contain values `0`, `1` and `2`, and the rest of the elements will be 
/// populated with `null` values.
///
function getXAxisRefs_Highcharts(minX, maxX, xValues)
{
   var xAxisRefs = [];

   for(let i = minX, k = 0; i <= maxX; i++) {
      if(k < xValues.length && i == xValues[k])
         xAxisRefs.push(k++);
      else
         xAxisRefs.push(null);
   }
   
   return xAxisRefs;   
}

///
/// @brief  Returns an array of Y axis values ranging inclusively from `maxX` to `minX`,
///         with elements that correspond to `xValues` values populated from the matching 
///         position in `yValues` and other elements set to `null`.
///
function getUsageData_Highcharts(minX, maxX, xValues, yValues)
{
   var series = [];

   for(let i = minX, k = 0; i <= maxX; i++) {
      if(k < xValues.length && i == xValues[k])
         series.push(yValues[k++]);
      else
         series.push(null);
   }
   
   return series;   
}

///
/// @brief  Returns an array of data points for the country usage pie chart series.
///
function getCountryUsageData_Highcharts(country_usage)
{
   var series = [];

   for(var i = 0; i < country_usage.getValueCount(); i++) {
      series.push({name: country_usage.getLabel(i), y: country_usage.getVisits(i)});
   }

   return series;
}

///
/// @brief  Returns an array of data points for the country usage map chart series.
///
function getCountryUsageData_HighchartsMap(country_usage)
{
   var series = [];

   for(var i = 0; i < country_usage.getValueCount(); i++) {
      // do not push the roll-up slice, which doesn't have a country code, into the map data array
      if(country_usage.getCCode(i).length)
         // Highmaps expects country codes in upper case
         series.push({ccode: country_usage.getCCode(i).toUpperCase(), 
                      value: country_usage.getVisits(i), 
                      color: country_usage.getColor(i)});
   }

   return series;
}

///
/// @brief  Returns an array of colors for the country usage pie chart.
///
function getCountryUsageColors_Highcharts(country_usage)
{
   var colors = [];

   for(var i = 0; i < country_usage.getValueCount(); i++) {
      colors.push(country_usage.getColor(i));
   }

   return colors;
}

///
/// @brief  Returns color-coded Hits/Files/Pages axis title markup with a shadow.
///
function getHitsTitleHtml_Highcharts(config, titles)
{
   // Highcharts translates this markup to SVG and does not support CSS classes, so the style has to be repeated
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.hits_color + "\">" + htmlEncode(titles.hits) + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.files_color + "\">" + htmlEncode(titles.files) + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.pages_color + "\">" + htmlEncode(titles.pages) + "</span>";
}

///
/// @brief  Returns color-coded Transfer axis title markup with a shadow.
///
function getXferTitleHtml_Highcharts(config, titles)
{
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.xfer_color + "\">" + htmlEncode(titles.xfer) + "</span>";
}

///
/// @brief  Returns color-coded Visits/Hosts axis title markup with a shadow
///
function getVisitsTitleHtml_Highcharts(config, titles)
{
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.visits_color + "\">" + htmlEncode(titles.visits) + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.hosts_color + "\">" + htmlEncode(titles.hosts) + "</span>";
}

///
/// @brief  Returns an array of short month names for the category axis of the monthly 
///         usage chart.
///
function getMonthlySummaryMonths_Highcharts(monthly_summary)
{
   var months = [];

   for(var i = 0; i < monthly_summary.monthCount; i++) {
      months.push(monthly_summary.shortMonths[(monthly_summary.firstMonth.month + i - 1) % 12]);
   }

   return months;
}

///
/// @brief  Returns an array of X axis references to the position of the series data 
///         in the original data array.
///
/// This function serves the same purpose as `getXAxisRefs_Highcharts`, but works with
/// compound X axis values consisting of objects `{year: number, month: number}`. If we
/// were to use a single function for plain numbers and compound objects, such function
/// would need to have access to abstract operations to increment, compare and assign
/// values, which would be unnecessarily complex for the purposes of this implementation.
///
function getMonthlyXAxisRefs_Highcharts(firstMonth, monthCount, months) 
{
   var xAxisRefs = [];
   var month = firstMonth.month;
   var year = firstMonth.year;

   //
   // `i` tracks elements of the returned X axis array and `k` tracks months in the 
   // input data array (`months`). 
   //
   for(let i = 0, k = 0; i < monthCount; i++) {
      if(k < months.length && months[k].month == month && months[k].year == year)
         xAxisRefs.push(k++);
      else
         xAxisRefs.push(null);

      if(month < 12)
         month++;
      else {
         month = 1;
         year++;
      }
   }

   return xAxisRefs;   
}

///
/// @brief  Returns of an array of Y axis values that positionally correspond to 
///         elements of the X axis value reference array (`xAxisRefs`). 
///
function getMonthlySummaryData_Highcharts(xAxisRefs, yValues)
{
   var series = [];

   for(let i = 0; i < xAxisRefs.length; i++) {
      if(xAxisRefs[i] != null)
         series.push(yValues[xAxisRefs[i]]);
      else
         series.push(null);
   }

   return series;   
}

//
// @brief   Returns an array of days from `1` to `lastDay`.
//
function getMonthDays(lastDay)
{
   var days = [];

   for(let i = 1; i <= lastDay; i++)
      days.push(i);

   return days;
}

///
/// @brief  Configures global Highcharts properties.
///
function setupCharts(config)
{
   Highcharts.setOptions({
      lang: {
         thousandsSep: "",             // disable the default separator (a space)
         numericSymbols: null          // disable numeric suffixes (kilo, mega, etc)
      }
   });

}

///
/// @brief  Renders a daily usage chart.
///
function renderDailyUsageChart(daily_usage)
{
   let xAxisRefs = getXAxisRefs_Highcharts(1, daily_usage.chart.maxDay, daily_usage.data.days); 

   var daily_usage_chart_options = {
      chart: {
         animation: false,
         backgroundColor: daily_usage.config.background_color,
         shadow: false,
         spacingLeft: 5,                        // axis titles add spacing
         spacingRight: 20
      },
      title: {
         align: "center",
         text: daily_usage.chart.title,
         style: { 
            color: daily_usage.config.title_color,
            fontSize: "16px" 
         }
      },
      legend: {
         enabled: false                         // disable legends the bottom of the chart
      },
      plotOptions: {
         column: {
            pointWidth: 7
         }
      },
      tooltip: {
         headerFormat: "<span style=\"font-size: 14px\">{point.key}</span><br/>"
      },
      series: [{
         animation: false,
         type: "column",
         color: daily_usage.config.hits_color,
         name: daily_usage.chart.seriesNames.hits,
         data: getUsageData_Highcharts(1, daily_usage.chart.maxDay, daily_usage.data.days, daily_usage.data.hits)
      },{
         animation: false,
         type: "column",
         color: daily_usage.config.files_color,
         name: daily_usage.chart.seriesNames.files,
         data: getUsageData_Highcharts(1, daily_usage.chart.maxDay, daily_usage.data.days, daily_usage.data.files)
      },{
         animation: false,
         type: "column",
         color: daily_usage.config.pages_color,
         name: daily_usage.chart.seriesNames.pages,
         data: getUsageData_Highcharts(1, daily_usage.chart.maxDay, daily_usage.data.days, daily_usage.data.pages)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: daily_usage.config.visits_color,
         name: daily_usage.chart.seriesNames.visits,
         data: getUsageData_Highcharts(1, daily_usage.chart.maxDay, daily_usage.data.days, daily_usage.data.visits)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: daily_usage.config.hosts_color,
         name: daily_usage.chart.seriesNames.hosts,
         data: getUsageData_Highcharts(1, daily_usage.chart.maxDay, daily_usage.data.days, daily_usage.data.hosts)
      },{
         animation: false,
         type: "column",
         yAxis: 2,
         color: daily_usage.config.xfer_color,
         name: daily_usage.chart.seriesNames.xfer,
         data: getUsageData_Highcharts(1, daily_usage.chart.maxDay, daily_usage.data.days, daily_usage.data.xfer),
         tooltip: {
            headerFormat: "<span style=\"font-size: 14px\">{point.key}</span><br/>",
            pointFormatter: function () 
            {
               return "<span style=\"color: " + this.color + "\">\u25CF</span> " + 
               htmlEncode(this.series.name) + ": <b>" + htmlEncode(daily_usage.data.xfer_hr[xAxisRefs[this.x]]) + "</b><br/>";
            }
         }
      }],
      //
      // The X axis is placed at the base of the plot area, which leaves sufficient room 
      // for labels, ticks and padding at the bottom of the frame. All charts, on the other 
      // hand, are placed at a distance measured from the top of the frame, so the bottom 
      // of the lowest chart may not align with the X axis. The X axis can be moved up by
      // assigning `offset` a negative value, but cannot be moved below the base line of the
      // plot area. If this is required, adjust the top property of the lowest `yAxis` object. 
      //
      // Highcharts needs X/Y pairs to render series with one value per data point, so they
      // manufacture X values as a zero-based sequence of values, which, essentially, contains
      // index values into the series data array. `xAxis.min` is used as a cut-off value for
      // X values and assigning `1` to `xAxis.min` doesn't mean that the X axis should start
      // with `1`, but rather that they should skip all X values up to `1`, which drops the
      // first day point for this chart and shifts all columns to the left. Use `categories`
      // instead to define day values and avoid setting `min`/`max` values, so the number of
      // columns is inferred from the size of the `categories` array.
      //
      xAxis: {
         offset: -4,
         allowDecimals: false,
         lineColor: daily_usage.config.gridline_color,
         tickColor: daily_usage.config.gridline_color,
         categories: getMonthDays(daily_usage.chart.maxDay),
         tickInterval: 1,
         labels: {
            step: 1,
            style: {fontSize: "11px"},
            formatter: function() 
            {
               if(daily_usage.isWeekend(this.value))
                  return "<span style=\"color: " + daily_usage.config.weekend_color + "; font-weight: bold\">" + this.value + "</span>";

               return this.value;

            }
         }
      },
      //
      // Daily usage chart is 450px high. Each Y-axis is 110px high and are stacked
      // with a 20px gap between them by setting their `top` property. The `offset`
      // property for each chart must be set to zero to prevent each axis shifted 
      // right to accommodate the next axis, which would be necessary if charts 
      // overlapped.
      //
      yAxis: [{
         height: 110,
         top: 40,
         offset: 0,
         ceiling: daily_usage.getDailyUsageMaxHits(), // use the largest hits value for the maximum auto-computed axis extreme
         allowDecimals: false,
         opposite: false,                       // render on the left side of the chart
         endOnTick: true,
         alignTicks: false,                     // not necessary because charts don't overlap
         gridLineColor: daily_usage.config.gridline_color,
         tickColor: daily_usage.config.gridline_color,
         lineColor: daily_usage.config.gridline_color,
         lineWidth: 1,                          // the default line width is zero
         showFirstLabel: false,
         reserveSpace: false,                   // no need to reserve space because labels are rendered inside the plot
         labels: {
            enabled: true,
            align: "left",
            formatter: function()               // show only the last label and without a suffix (e.g. k for kilo, etc)
            {
               return (this.isLast) ? this.value : null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getHitsTitleHtml_Highcharts(daily_usage.config, daily_usage.chart.seriesNames)
         }
      },{
         height: 110,
         top: 170,
         offset: 0,
         max: daily_usage.getDailyUsageMaxVisits(),
         allowDecimals: false,
         opposite: false,
         endOnTick: true,
         alignTicks: false,
         gridLineColor: daily_usage.config.gridline_color,
         tickColor: daily_usage.config.gridline_color,
         lineWidth: 1,
         lineColor: daily_usage.config.gridline_color,
         showFirstLabel: false,
         reserveSpace: false,
         labels: {
            enabled: true,
            align: "left",
            formatter: function() 
            {
               return (this.isLast) ? this.value : null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getVisitsTitleHtml_Highcharts(daily_usage.config, daily_usage.chart.seriesNames)
         }
      },{
         height: 110,
         top: 299,                              // should be 300; adjusted to align with the X axis
         offset: 0,
         max: daily_usage.getDailyUsageMaxXfer(),
         allowDecimals: false,
         opposite: false,
         endOnTick: true,
         alignTicks: false,
         gridLineColor: daily_usage.config.gridline_color,
         tickColor: daily_usage.config.gridline_color,
         lineWidth: 1,
         lineColor: daily_usage.config.gridline_color,
         showFirstLabel: false,
         reserveSpace: false,
         labels: {
            enabled: true,
            align: "left",
            formatter: function() 
            {
               if(this.isLast)
                  return formatAxisLabel(this.value, " ", daily_usage.config);

               return null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getXferTitleHtml_Highcharts(daily_usage.config, daily_usage.chart.seriesNames)
         }
      }]
   };

   Highcharts.chart("daily_usage_chart", daily_usage_chart_options);
}

///
/// #brief  Renders an hourly usage chart.
///
function renderHourlyUsageChart(hourly_usage)
{
   let xAxisRefs = getXAxisRefs_Highcharts(0, 23, hourly_usage.data.hours); 

   var hourly_usage_chart_options = {
      chart: {
         animation: false,
         backgroundColor: hourly_usage.config.background_color,
         shadow: false,
         spacingLeft: 5,
         spacingRight: 20
      },
      title: {
         align: "center",
         text: hourly_usage.chart.title,
         style: { 
            color: hourly_usage.config.title_color,
            fontSize: "16px" 
         }
      },
      legend: {
         enabled: false
      },
      plotOptions: {
         column: {
            pointWidth: 7
         }
      },
      tooltip: {
         headerFormat: "<span style=\"font-size: 14px\">{point.key}</span><br/>"
      },
      series: [{
         animation: false,
         type: "column",
         color: hourly_usage.config.hits_color,
         name: hourly_usage.chart.seriesNames.hits,
         data: getUsageData_Highcharts(0, 23, hourly_usage.data.hours, hourly_usage.data.hits)
      },{
         animation: false,
         type: "column",
         color: hourly_usage.config.files_color,
         name: hourly_usage.chart.seriesNames.files,
         data: getUsageData_Highcharts(0, 23, hourly_usage.data.hours, hourly_usage.data.files)
      },{
         animation: false,
         type: "column",
         color: hourly_usage.config.pages_color,
         name: hourly_usage.chart.seriesNames.pages,
         data: getUsageData_Highcharts(0, 23, hourly_usage.data.hours, hourly_usage.data.pages)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: hourly_usage.config.xfer_color,
         name: hourly_usage.chart.seriesNames.xfer,
         data: getUsageData_Highcharts(0, 23, hourly_usage.data.hours, hourly_usage.data.xfer),
         tooltip: {
            headerFormat: "<span style=\"font-size: 14px\">{point.key}</span><br/>",
            pointFormatter: function () 
            {
               return "<span style=\"color: " + this.color + "\">\u25CF</span> " + 
               htmlEncode(this.series.name) + ": <b>" + htmlEncode(hourly_usage.data.xfer_hr[xAxisRefs[this.x]]) + "</b><br/>";
            }
         }
      }],
      xAxis: {
         offset: -4,
         allowDecimals: false,
         lineColor: hourly_usage.config.gridline_color,
         tickColor: hourly_usage.config.gridline_color,
         min: 0,
         max: 23,
         tickInterval: 1,
         labels: {
            step: 1,
            style: {fontSize: "11px"}
         }
      },
      yAxis: [{
         height: 200,
         top: 40,
         offset: 0,
         ceiling: hourly_usage.getHourlyUsageMaxHits(),
         allowDecimals: false,
         opposite: false,
         endOnTick: true,
         alignTicks: false,
         gridLineColor: hourly_usage.config.gridline_color,
         tickColor: hourly_usage.config.gridline_color,
         lineColor: hourly_usage.config.gridline_color,
         lineWidth: 1,
         showFirstLabel: false,
         reserveSpace: false,
         labels: {
            enabled: true,
            align: "left",
            formatter: function() 
            {
               return (this.isLast) ? this.value : null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getHitsTitleHtml_Highcharts(hourly_usage.config, hourly_usage.chart.seriesNames)
         }
      },{
         height: 150,
         top: 259,                              // should be 260; adjusted to align with the X axis
         offset: 0,
         max: hourly_usage.getHourlyUsageMaxXfer(),
         allowDecimals: false,
         opposite: false,
         endOnTick: true,
         alignTicks: false,
         gridLineColor: hourly_usage.config.gridline_color,
         tickColor: hourly_usage.config.gridline_color,
         lineWidth: 1,
         lineColor: hourly_usage.config.gridline_color,
         showFirstLabel: false,
         reserveSpace: false,
         labels: {
            enabled: true,
            align: "left",
            formatter: function() 
            {
               if(this.isLast)
                  return formatAxisLabel(this.value, " ", hourly_usage.config);

               return null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getXferTitleHtml_Highcharts(hourly_usage.config, hourly_usage.chart.seriesNames)
         }
      }]
   };

   Highcharts.chart("hourly_usage_chart", hourly_usage_chart_options);
}

///
/// @brief  Renders a country usage pie chart.
///
function renderCountryUsageChart(country_usage)
{
   var country_usage_chart_options = {
         chart: {
         animation: false,
         backgroundColor: country_usage.config.background_color,
         shadow: false
      },
      title: {
         align: "center",
         text: country_usage.chart.title,
         style: { 
            color: country_usage.config.title_color,
            fontSize: "16px" 
         }
      },
      legend: {
         enabled: true,
         layout: "vertical",
         align: "right",
         verticalAlign: "middle",
         itemMarginBottom: 10,                  // in pixels
         labelFormatter: function ()
         {
            return this.name + 
               " <span style=\"font-weight: normal\">(" + 
                  country_usage.getPercent(this.x) + 
               ")</span>";

         }
      },
      series: [{
         type: "pie",
         size: 280,
         colors: getCountryUsageColors_Highcharts(country_usage),
         shadow: true,
         animation: false,
         showInLegend: true,
         startAngle: 90,
         borderColor: "black",
         name: country_usage.chart.seriesNames.visits,
         data: getCountryUsageData_Highcharts(country_usage),
         dataLabels: {
            enabled: false,
            reserveSpace: false,
         },
         tooltip: {
            headerFormat: "<span style=\"font-size: 14px\">{point.key}</span><br/>",
            pointFormatter: function()
            {
               return "<span style=\"color:" + this.color + "\">\u25CF</span> " + htmlEncode(this.series.name) + 
                  " <b>" + this.y + "</b> (" + country_usage.getPercent(this.x) + ")";
            }
         },
         point: {
            events: {
               legendItemClick: function(e) 
               {
                  // prevent the default click action that hides the slice
                  e.preventDefault();
               }
            }
         }
      }]
   };


   Highcharts.chart("country_usage_chart", country_usage_chart_options);
}

///
/// @brief  Renders a country usage map chart.
///
function renderCountryUsageChartMap(country_usage)
{
   var country_usage_chart_options = {
      chart: {
         animation: false,
         backgroundColor: country_usage.config.background_color,
         shadow: false
      },
      title: {
         align: "center",
         text: country_usage.chart.title,
         style: {
            color: country_usage.config.title_color,
            fontSize: "16px"
         }
      },
      legend: {
         enabled: false
      },
      mapNavigation: {
         enabled: true,
         buttonOptions: {
            verticalAlign: "bottom"
         }
      },
      plotOptions: {
         map: {
            allAreas: true,
            shadow: false
         }
      },
      tooltip: {
         useHTML: true,                         // cannot be set in the series tooltip
      },
      series: [{
         type: "map",
         mapData: Highcharts.maps["custom/world"],
         joinBy: ["iso-a2", "ccode"],
         showInLegend: false,                   // use tooltips instead
         borderColor: "black",
         name: country_usage.chart.seriesNames.visits,
         data: getCountryUsageData_HighchartsMap(country_usage),
         dataLabels: {
            enabled: true,
            reserveSpace: true,
         },
         tooltip: {
            animation: false,
            borderWidth: 1,
            followPointer: true,                // if not set, tooltip may be not visible on a zoomed in map
            shadow: false,
            padding: 0,
            headerFormat: "",                   // cannot use {point.key} because we need our country names
            pointFormatter: function() 
            {
               // use the country name from country_usage for consistency
               return "<span style=\"font-weight: normal; font-size: 14px\">" +
                  htmlEncode(country_usage.getLabel(this.x)) + " " + "</span><br/>" + 
                  "<span style=\"color: " + this.color + "\">&#x25CF;</span> " + 
                  "<span style=\"font-size: 14px\">" + this.series.name + 
                  " <b>" + this.value + "</b> (" + country_usage.getPercent(this.x) + ")</span>";
            }
         }
      }]
   };


   Highcharts.mapChart("country_usage_chart", country_usage_chart_options);
}

///
/// @brief  Renders a monthly summary chart.
///
function renderMonthlySummaryChart(monthly_summary)
{
   var xAxisRefs = getMonthlyXAxisRefs_Highcharts(monthly_summary.chart.firstMonth, monthly_summary.chart.monthCount, monthly_summary.data.months);

   var monthly_summary_chart_options = {
      chart: {
         animation: false,
         backgroundColor: monthly_summary.config.background_color,
         shadow: false,
         spacingLeft: 5,
         spacingRight: 20
      },
      title: {
         align: "center",
         text: monthly_summary.chart.title,
         style: { 
            color: monthly_summary.config.title_color,
            fontSize: "16px" 
         }
      },
      legend: {
         enabled: false
      },
      plotOptions: {
         column: {
            pointWidth: 10
         }
      },
      tooltip: {
         //
         // The tooltip formatter function allows us to output a custom point header that
         // lists a full month and a year. However, having this function defined, disables 
         // individual series pointFormatter callbacks.
         //
         formatter: function ()
         {
            // compute a zero-based offset in months from January of the first history year 
            var monthOffset = monthly_summary.chart.firstMonth.month + this.point.x - 1;
            var tooltipText;
            
            // for the transfer amount series (5) use human-readable transfer values when available
            if(this.series.index == 5)
               tooltipText = htmlEncode(monthly_summary.data.xfer_hr[xAxisRefs[this.point.x]]);
            else
               tooltipText = this.point.y;

            // output full month and year before point data instead of the standard point header
            return "<span style=\"font-size: 12px\">" + htmlEncode(monthly_summary.chart.longMonths[monthOffset % 12]) + " " + 
               (monthly_summary.chart.firstMonth.year + Math.floor(monthOffset / 12)) + "</span><br/>" + 
               "<span style=\"color:" + this.point.color + "\">\u25CF</span> " + 
               htmlEncode(this.series.name) + ": <b>" + tooltipText + "</b><br/>";
         }
      },
      series: [{
         animation: false,
         type: "column",
         color: monthly_summary.config.hits_color,
         name: monthly_summary.chart.seriesNames.hits,
         data: getMonthlySummaryData_Highcharts(xAxisRefs, monthly_summary.data.hits)
      },{
         animation: false,
         type: "column",
         color: monthly_summary.config.files_color,
         name: monthly_summary.chart.seriesNames.files,
         data: getMonthlySummaryData_Highcharts(xAxisRefs, monthly_summary.data.files)
      },{
         animation: false,
         type: "column",
         color: monthly_summary.config.pages_color,
         name: monthly_summary.chart.seriesNames.pages,
         data: getMonthlySummaryData_Highcharts(xAxisRefs, monthly_summary.data.pages)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: monthly_summary.config.visits_color,
         name: monthly_summary.chart.seriesNames.visits,
         data: getMonthlySummaryData_Highcharts(xAxisRefs, monthly_summary.data.visits)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: monthly_summary.config.hosts_color,
         name: monthly_summary.chart.seriesNames.hosts,
         data: getMonthlySummaryData_Highcharts(xAxisRefs, monthly_summary.data.hosts)
      },{
         animation: false,
         type: "column",
         yAxis: 2,
         color: monthly_summary.config.xfer_color,
         name: monthly_summary.chart.seriesNames.xfer,
         data: getMonthlySummaryData_Highcharts(xAxisRefs, monthly_summary.data.xfer)
      }],
      //
      // This is a category axis that lists only short month names. The numeric value of 
      // point.x is in the range from zero to the history length minus one. See the tooltip 
      // formatter function in this chart for an example on how to figure out month names 
      // and numbers.
      //
      xAxis: {
         type: "category",
         offset: -4,
         tickAmount: monthly_summary.chart.monthCount,
         categories: getMonthlySummaryMonths_Highcharts(monthly_summary.chart),
         allowDecimals: false,
         lineColor: monthly_summary.config.gridline_color,
         tickColor: monthly_summary.config.gridline_color,
         tickInterval: 1,
         labels: {
            step: 1,
            style: {fontSize: "11px"}
         }
      },
      yAxis: [{
         height: 110,
         top: 40,
         offset: 0,
         ceiling: monthly_summary.getMonthlySummaryMaxHits(),
         allowDecimals: false,
         opposite: false,
         endOnTick: true,
         alignTicks: false,
         gridLineColor: monthly_summary.config.gridline_color,
         tickColor: monthly_summary.config.gridline_color,
         lineColor: monthly_summary.config.gridline_color,
         lineWidth: 1,
         showFirstLabel: false,
         reserveSpace: false,
         labels: {
            enabled: true,
            align: "left",
            formatter: function()
            {
               return (this.isLast) ? this.value : null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getHitsTitleHtml_Highcharts(monthly_summary.config, monthly_summary.chart.seriesNames)
         }
      },{
         height: 110,
         top: 170,
         offset: 0,
         max: monthly_summary.getMonthlySummaryMaxVisits(),
         allowDecimals: false,
         opposite: false,
         endOnTick: true,
         alignTicks: false,
         gridLineColor: monthly_summary.config.gridline_color,
         tickColor: monthly_summary.config.gridline_color,
         lineWidth: 1,
         lineColor: monthly_summary.config.gridline_color,
         showFirstLabel: false,
         reserveSpace: false,
         labels: {
            enabled: true,
            align: "left",
            formatter: function() 
            {
               return (this.isLast) ? this.value : null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getVisitsTitleHtml_Highcharts(monthly_summary.config, monthly_summary.chart.seriesNames)
         }
      },{
         height: 110,
         top: 299,                                 // should be 300; adjusted to align with the X axis
         offset: 0,
         max: monthly_summary.getMonthlySummaryMaxXfer(),
         allowDecimals: false,
         opposite: false,
         endOnTick: true,
         alignTicks: false,
         gridLineColor: monthly_summary.config.gridline_color,
         tickColor: monthly_summary.config.gridline_color,
         lineWidth: 1,
         lineColor: monthly_summary.config.gridline_color,
         showFirstLabel: false,
         reserveSpace: false,
         labels: {
            enabled: true,
            align: "left",
            formatter: function() 
            {
               if(this.isLast)
                  return formatAxisLabel(this.value, " ", monthly_summary.config);

               return null;
            },
            y: 12,
            x: 3
         },
         title: {
            align: "middle",
            fontWeight: "bold",
            text: getXferTitleHtml_Highcharts(monthly_summary.config, monthly_summary.chart.seriesNames)
         }
      }]
   };
   
   Highcharts.chart("monthly_summary_chart", monthly_summary_chart_options);
}
