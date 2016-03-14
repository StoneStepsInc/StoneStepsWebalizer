/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

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
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.hits_color + "\">" + titles.hits + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.files_color + "\">" + titles.files + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.pages_color + "\">" + titles.pages + "</span>";
}

//
// Returns color-coded Transfer axis title markup with a shadow
//
function getXferTitleHtml_Highcharts(config, titles)
{
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.xfer_color + "\">" + titles.xfer + "</span>";
}

//
// Returns color-coded Visits/Hosts axis title markup with a shadow
//
function getVisitsTitleHtml_Highcharts(config, titles)
{
   return "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.visits_color + "\">" + titles.visits + "</span> / " +
      "<span style=\"text-shadow: 1px 1px 0 #777; color: " + config.hosts_color + "\">" + titles.hosts + "</span>";
}

//
// Render a daily usage chart in a predefined div element in a report
//
function renderDailyUsageChart_Highcharts(daily_usage)
{
   var daily_usage_chart_options = {
      lang: {
         numericSymbols: null                   // disable numeric suffixes (kilo, mega, etc)
      },
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
         data: getUsageData_Highcharts(daily_usage.data.days, daily_usage.data.xfer)
      }],
      //
      // The X axis is placed at a sufficient distance from the bottom of the frame 
      // to accommodate labels, ticks and padding. All plots, on the other hand, are
      // placed at a distance measured from the top of the frame, so the bottom of
      // the lowest plot doesn't align with the X axis and the latter needs to be
      // moved down a few pixels, depending on the chart frame size, label font size, 
      // etc. Given current settings, the offset is two pixels.
      //
      xAxis: {
         offset: -2,
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
      // Daily usage chart is 450px high. Each Y-axis is 110px high and are stcked
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
               if(this.isLast) return this.value;
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
               if(this.isLast) return this.value;
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
         top: 300,
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
               if(this.isLast) return this.value;
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

//
// Render a daily usage chart in a predefined div element in a report
//
function renderHourlyUsageChart_Highcharts(hourly_usage)
{
   var hourly_usage_chart_options = {
      lang: {
         numericSymbols: null
      },
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
         data: getUsageData_Highcharts(hourly_usage.data.hours, hourly_usage.data.xfer)
      }],
      xAxis: {
         offset: -2,
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
               if(this.isLast) return this.value;
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
         top: 260,
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
               if(this.isLast) return this.value;
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

function renderCountryUsageChart_Highcharts(country_usage)
{
   var country_usage_chart_options = {
      lang: {
         numericSymbols: null
      },
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
         itemMarginBottom: 15,
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
            pointFormatter: function()
            {
               return "<span style=\"color:" + this.color + "\">\u25CF</span> " + this.series.name + 
                  " <b>" + this.y + "</b> (" + country_usage.getPercent(this.x) + ")";
            }
         },
         point: {
            events: {
               legendItemClick: function(e) 
               {
                  // prevent the default action that hides the slice
                  e.preventDefault();
               }
            }
         }
      }]
   };


   Highcharts.chart("country_usage_chart", country_usage_chart_options);
}
