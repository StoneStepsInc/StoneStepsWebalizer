/*
   webalizer - a web server log analysis program

   Copyright (c) 2016-2017, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     
   
   webalizer_highcharts.js
*/

//
// Converts usage data arrays xValues and yValues into a Highcharts series
// array populated with [x,y] data points
//
function getUsageData_Highcharts(xValues, yValues)
{
   var series = [];

   for(var i = 0; i < xValues.length; i++) {
      series.push([xValues[i], yValues[i]]);
   }
   
   return series;   
}

//
// Converts country usage data arrays into a Highcharts pie chart series data 
// point array.
//
function getCountryUsageData_Highcharts(country_usage)
{
   var series = [];

   for(var i = 0; i < country_usage.getValueCount(); i++) {
      series.push({name: country_usage.getLabel(i), y: country_usage.getVisits(i)});
   }

   return series;
}

function getCountryUsageData_HighchartsMap(country_usage)
{
   var series = [];

   for(var i = 0; i < country_usage.getValueCount(); i++) {
      // do not push the roll-up slice into the map data array
      if(country_usage.getCCode(i).length)
         // Highmaps expects countr codes in upper case
         series.push({ccode: country_usage.getCCode(i).toUpperCase(), 
                      value: country_usage.getVisits(i), 
                      color: country_usage.getColor(i)});
   }

   return series;
}

//
// Returns an array of colors for the country usage pie chart
//
function getCountryUsageColors_Highcharts(country_usage)
{
   var colors = [];

   for(var i = 0; i < country_usage.getValueCount(); i++) {
      colors.push(country_usage.getColor(i));
   }

   return colors;
}

//
// Returns color-coded Hits/Files/Pages axis title markup with a shadow
//
function getHitsTitleHtml_Highcharts(config, titles)
{
   // Highcharts translates this markup to SVG and does not support CSS classes, so the style has to be repeated
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.hits_color + "\">" + htmlEncode(titles.hits) + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.files_color + "\">" + htmlEncode(titles.files) + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.pages_color + "\">" + htmlEncode(titles.pages) + "</span>";
}

//
// Returns color-coded Transfer axis title markup with a shadow
//
function getXferTitleHtml_Highcharts(config, titles)
{
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.xfer_color + "\">" + htmlEncode(titles.xfer) + "</span>";
}

//
// Returns color-coded Visits/Hosts axis title markup with a shadow
//
function getVisitsTitleHtml_Highcharts(config, titles)
{
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.visits_color + "\">" + htmlEncode(titles.visits) + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.hosts_color + "\">" + htmlEncode(titles.hosts) + "</span>";
}

//
// Returns an array of short month names for the category axis of the summary usage chart
//
function getMonthlySummaryMonths_Highcharts(monthly_summary)
{
   var months = [];

   for(var i = 0; i < monthly_summary.monthCount; i++) {
      months.push(monthly_summary.shortMonths[(monthly_summary.firstMonth.month + i - 1) % 12]);
   }

   return months;
}

//
// Returns a series array containing Y axis values positionally corresponding
// to the X axis label array returned from getMonthlySummaryMonths_Highcharts.
//
function getMonthlySummaryData_Highcharts(monthly_summary, months, yValues)
{
   var series = [];
   var month = monthly_summary.firstMonth.month;
   var year = monthly_summary.firstMonth.year;

   //
   // The series array must contain as many elements as there are axis 
   // categories. Those series elements that don't have any data values
   // will be filed with null values.
   //
   // Start with the actual first month in the report summary data and
   // iterate once for every month in the series (`i`). If the series 
   // year and the month match those in the `months` array, assign the
   // series data point and move to the next summary month (`k`). The 
   // loop ends when we run out of values in `months` or populated all 
   // series months.
   //
   for(var i = 0, k = 0; k < months.length && i < monthly_summary.monthCount; i++) {
      
      if(months[k].month == month && months[k].year == year)
         series.push(yValues[k++]);
      else
         series.push(null);

      if(month == 12) {
         month = 1;
         year++;
      }
      else
         month++;
   }

   //
   // Ideally, we could just set `series.length` to the total number of months
   // and then just assign those elements that correspond to the values in the
   // `months` array, but Highcharts ignores array elements with `undefined` 
   // values, so we need to fill up the rest of the series with `null` values.
   //
   for(var i = series.length; i < monthly_summary.monthCount; i++)
      series.push(null);

   return series;   
}

//
// Configure global Highcharts properties
//
function setupCharts(config)
{
   Highcharts.setOptions({
      lang: {
         thousandsSep: "",             // disable the default separator (a space)
         numericSymbols: null          // disable numeric suffixes (kilo, mega, etc)
      }
   });

}

//
// Daily usage chart
//

function renderDailyUsageChart(daily_usage)
{
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
         data: getUsageData_Highcharts(daily_usage.data.days, daily_usage.data.hits)
      },{
         animation: false,
         type: "column",
         color: daily_usage.config.files_color,
         name: daily_usage.chart.seriesNames.files,
         data: getUsageData_Highcharts(daily_usage.data.days, daily_usage.data.files)
      },{
         animation: false,
         type: "column",
         color: daily_usage.config.pages_color,
         name: daily_usage.chart.seriesNames.pages,
         data: getUsageData_Highcharts(daily_usage.data.days, daily_usage.data.pages)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: daily_usage.config.visits_color,
         name: daily_usage.chart.seriesNames.visits,
         data: getUsageData_Highcharts(daily_usage.data.days, daily_usage.data.visits)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: daily_usage.config.hosts_color,
         name: daily_usage.chart.seriesNames.hosts,
         data: getUsageData_Highcharts(daily_usage.data.days, daily_usage.data.hosts)
      },{
         animation: false,
         type: "column",
         yAxis: 2,
         color: daily_usage.config.xfer_color,
         name: daily_usage.chart.seriesNames.xfer,
         data: getUsageData_Highcharts(daily_usage.data.days, daily_usage.data.xfer),
         tooltip: {
            headerFormat: "<span style=\"font-size: 14px\">{point.key}</span><br/>",
            pointFormatter: function () 
            {
               return "<span style=\"color: " + this.color + "\">\u25CF</span> " + 
               htmlEncode(this.series.name) + ": <b>" + htmlEncode(daily_usage.data.xfer_hr[this.x-1]) + "</b><br/>";
            }
         }
      }],
      //
      // The X axis is placed at the base of the plot area, which leaves sufficient room 
      // for labels, ticks and padding at the bottom of the frame. All charts, on the other 
      // hand, are placed at a distance measured from the top of the frame, so the bottom 
      // of the lowest chart may not align with the X axis. The X axis can be moved up by
      // assigning offset a negative value, but cannot be moved below the base line of the
      // plot area. If this is required, adjust the top property of the lowest yAxis object. 
      //
      xAxis: {
         offset: 0,
         allowDecimals: false,
         lineColor: daily_usage.config.gridline_color,
         tickColor: daily_usage.config.gridline_color,
         min: 1,
         max: daily_usage.chart.maxDay,
         tickInterval: 1,
         labels: {
            step: 1,
            style: {fontSize: "11px"},
            formatter: function() 
            {
               if(daily_usage.isWeekend(this.value))
                  return "<span style=\"color: " + daily_usage.config.weekend_color + "\">" + this.value + "</span>";

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

//
// Hourly usage chart
//

function renderHourlyUsageChart(hourly_usage)
{
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
         data: getUsageData_Highcharts(hourly_usage.data.hours, hourly_usage.data.hits)
      },{
         animation: false,
         type: "column",
         color: hourly_usage.config.files_color,
         name: hourly_usage.chart.seriesNames.files,
         data: getUsageData_Highcharts(hourly_usage.data.hours, hourly_usage.data.files)
      },{
         animation: false,
         type: "column",
         color: hourly_usage.config.pages_color,
         name: hourly_usage.chart.seriesNames.pages,
         data: getUsageData_Highcharts(hourly_usage.data.hours, hourly_usage.data.pages)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: hourly_usage.config.xfer_color,
         name: hourly_usage.chart.seriesNames.xfer,
         data: getUsageData_Highcharts(hourly_usage.data.hours, hourly_usage.data.xfer),
         tooltip: {
            headerFormat: "<span style=\"font-size: 14px\">{point.key}</span><br/>",
            pointFormatter: function () 
            {
               return "<span style=\"color: " + this.color + "\">\u25CF</span> " + 
               htmlEncode(this.series.name) + ": <b>" + htmlEncode(hourly_usage.data.xfer_hr[this.x]) + "</b><br/>";
            }
         }
      }],
      xAxis: {
         offset: 0,
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

//
// Country usage chart
//

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

//
// Monthly summary chart
//

function renderMonthlySummaryChart(monthly_summary)
{
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
         // individual series pointFormatter calbacks.
         //
         formatter: function ()
         {
            // compute a zero-based offset in months from January of the first history year 
            var monthOffset = monthly_summary.chart.firstMonth.month + this.point.x - 1;
            var tooltipText;
            
            // for the transfer amount series (5) use human-readable transfer values when available
            if(this.series.index == 5 && monthly_summary.data.xfer_hr[this.point.x])
               tooltipText = htmlEncode(monthly_summary.data.xfer_hr[this.point.x]);
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
         data: getMonthlySummaryData_Highcharts(monthly_summary.chart, monthly_summary.data.months, monthly_summary.data.hits)
      },{
         animation: false,
         type: "column",
         color: monthly_summary.config.files_color,
         name: monthly_summary.chart.seriesNames.files,
         data: getMonthlySummaryData_Highcharts(monthly_summary.chart, monthly_summary.data.months, monthly_summary.data.files)
      },{
         animation: false,
         type: "column",
         color: monthly_summary.config.pages_color,
         name: monthly_summary.chart.seriesNames.pages,
         data: getMonthlySummaryData_Highcharts(monthly_summary.chart, monthly_summary.data.months, monthly_summary.data.pages)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: monthly_summary.config.visits_color,
         name: monthly_summary.chart.seriesNames.visits,
         data: getMonthlySummaryData_Highcharts(monthly_summary.chart, monthly_summary.data.months, monthly_summary.data.visits)
      },{
         animation: false,
         type: "column",
         yAxis: 1,
         color: monthly_summary.config.hosts_color,
         name: monthly_summary.chart.seriesNames.hosts,
         data: getMonthlySummaryData_Highcharts(monthly_summary.chart, monthly_summary.data.months, monthly_summary.data.hosts)
      },{
         animation: false,
         type: "column",
         yAxis: 2,
         color: monthly_summary.config.xfer_color,
         name: monthly_summary.chart.seriesNames.xfer,
         data: getMonthlySummaryData_Highcharts(monthly_summary.chart, monthly_summary.data.months, monthly_summary.data.xfer)
      }],
      //
      // This is a category axis that lists only short month names. The numeric value of 
      // point.x is in the range from zero to the history length minus one. See the tooltip 
      // formatter function in this chart for an example on how to figure out month names 
      // and numbers.
      //
      xAxis: {
         type: "category",
         offset: 0,
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
