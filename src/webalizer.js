/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-1013, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     
   
	webalizer.js
*/
var ELEMENT_NODE  = 1;        // DOM element node type
var TEXT_NODE     = 3;

var KEY_ESC       = 27;
var KEY_HOME      = 36;
var KEY_END       = 35;
var KEY_PAGEUP    = 33;
var KEY_PAGEDOWN  = 34;
var KEY_UP        = 38;
var KEY_DOWN      = 40;
var KEY_LEFT      = 37;
var KEY_RIGHT     = 39;

var PAGE_INDEX    = 0
var PAGE_USAGE    = 1;

var ie6 = false;
var ie7 = false;
var ie8 = false;
var ie9 = false;
var allitems = null;          // instance of ViewAllInfo, if there is an expanded report
var frags = null;             // report identifiers (fragments)
var helpbox = null;           // help box div
var helptext = null;          // hidden help text container div

function ViewAllInfo()
{
   this.rptlink = null;       // a link in front of each report
   this.viewall = null;       // view all link (tbody)
   this.allitems = null;      // all items (tbody)
   this.titlenode = null;     // title table cell (th)
   this.title = null;         // original report title (String)
}

function onloadpage(pagetype)
{
   // check for versions of IE
   ie6 = /MSIE[ \t]+6/.test(window.navigator.userAgent);
   ie7 = /MSIE[ \t]+7/.test(window.navigator.userAgent);
   ie8 = /MSIE[ \t]+8/.test(window.navigator.userAgent);
   ie9 = /MSIE[ \t]+9/.test(window.navigator.userAgent);
   
   // update the fragment array
   if(pagetype == PAGE_USAGE)
      updateFragments();
   
   // find the help box div
   if((helpbox = document.getElementById("helpbox")) != null) 
      helpbox.normalize();
   
   // find the help container div
   helptext = document.getElementById("helptext");
}

function onclickmenu(a)
{
	return true;
}

//
// onpagekeyup
//
// Handles page-level keyboard events
//
function onpagekeyup(event)
{
   // check if it's the ESC key
   if(event.keyCode == KEY_ESC) {
      // if the help is visible, hide it and return
      if(isHelpShown()) {
         hideHelp();
         return;
      }
      
      hideAllItems(true);
   }
   
   // if frags is initialized and
   if(frags) {
      // if Ctrl and Alt are down, process navigation keys
      if(event.ctrlKey && event.altKey) {
         switch(event.keyCode) {
            case KEY_HOME:
               document.location.href = "#" + frags[0];
               break;

            case KEY_END:
               document.location.href = "#" + frags[frags.length-1];
               break;
               
            case KEY_UP:
               document.location.href = "#" + getPrevFragId(getFragId(document.location.href));
               break;

            case KEY_DOWN:
               document.location.href = "#" + getNextFragId(getFragId(document.location.href));
               break;
         }
      }
   }
   
   return true;
}

//
// updateFragments
//
// Populates the fragment array with fragment identifiers from the 
// main menu
//
function updateFragments()
{
   var menu = document.getElementById("main_menu");
   var links;

   // initialize the fragment array
   frags = ["top"];
   
   // index page doesn't have a menu
   if(menu) {
      links = menu.getElementsByTagName("a");
      
      if(links) {
         for(var i = 0; i < links.length; i++)
            frags.push(getFragId(links.item(i).href));
      }
   }
}

//
// getFragId
//
// Returns the fragment identifier in href or an empty string if there 
// is none.
//
function getFragId(href)
{
   var fpos = href? href.lastIndexOf("#") : -1;
   return fpos != -1 ? href.substr(fpos+1) : "";
}

//
// getPrevFragId
//
// Finds fragid in the frags array and returns the previous element. If
// fragid is not found, returns the last element of the array.
//
function getPrevFragId(fragid)
{
   if(fragid) {
      for(var i = 1; i < frags.length; i++) {
         if(frags[i] == fragid)
            return frags[i-1];
      }
   }
   
   return frags[frags.length-1];
}

//
// getNextFragId
//
// Finds fragid in the frags array and returns the next element. If fragid
// is not found, returns the first element of the array.
//
function getNextFragId(fragid)
{
   if(fragid) {
      for(var i = 0; i < frags.length-1; i++) {
         if(frags[i] == fragid)
            return frags[i+1];
      }
   }
   
   return frags[0];
}

//
// hideAllItems
//
// Collapse an expanded all-items section, if there's any
//
function hideAllItems(gotorep)
{
   if(allitems) {
      // make the View All link visible
      allitems.viewall.style.display = (ie6 || ie7 || ie8 || ie9) ? "block" : "table-row-group";
      
      // hide the remaining items
      allitems.allitems.style.display = "none";

      // and restore the report title
      allitems.titlenode.firstChild.data = allitems.title;
      
      // if requested, go to the top of the report
      if(gotorep)
         document.location.href = "#" + allitems.rptlink.name;
         
      // all done, reset the descriptor
      allitems = null;
   }
}

//
// findReportTitle
//
// Given any node inside a report table, returns the report title cell.
// The text data within the cell is normalized before the call returns,
// so it is possible to use firstChild against the returned value to
// access title text.
//
function findReportTitle(node)
{
   // find the parent table
   node = findParentNode(node, "table");
   
   // find the head section
   node = node.firstChild;

   while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != "thead")
      node = node.nextSibling;

   // find the title row
   node = node.firstChild;

   while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != "tr" || node.className.indexOf("table_title_tr") == -1)
      node = node.nextSibling;
   
   // find the title cell
   node = node.firstChild;

   while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != "th")
      node = node.nextSibling;
   
   node.normalize();
   
   return node;
}   

//
// findParentNode
//
// Returns the parent node with the matching tag name of the specified node
//
function findParentNode(node, tagname)
{
   do {
      node = node.parentNode;
   } while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != tagname);
   
   return node;
}

//
// findNextSibling
//
// Returns the next sibling node with the matching tag name of the specified node
//
function findNextSibling(node, tagname)
{
   do {
      node = node.nextSibling;
   } while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != tagname);
   
   return node;
}

//
// findPrevSibling
//
// Returns the previous sibling node with the matching tag name of the specified node
//
function findPrevSibling(node, tagname)
{
   do {
      node = node.previousSibling;
   } while(node.nodeType != ELEMENT_NODE || node.tagName.toLowerCase() != tagname);
   
   return node;
}

//
// showAllItems
//
// Shows the hidden table section with the remainder of report items,
// fixes up the title and hides the View All table section.
//
function showAllItems(node, top, count)
{
   if(!node)
      return false;

   // hide the expanded section, if there's any
   hideAllItems(false);
   
   // create an empty array to hold table sections
   allitems = new ViewAllInfo();
   
   //
   // Hide the table section containing the View All link
   //
   node = findParentNode(node, "tbody")

   // store the link node
   allitems.viewall = node;
   
   // and hide the section
   node.style.display = "none";
   
   //
   // Expand the section containing remaining report items
   //
   node = findNextSibling(node, "tbody");

   // store it, so we can use it later to collapse the section
   allitems.allitems = node;

   // and make the section visible (IE6/7 do not understand table-row-group)
   node.style.display = (ie6 || ie7 || ie8 || ie9) ? "block" : "table-row-group";

   //
   // Fix the title to reflect the number of displayed items
   //
   allitems.titlenode = findReportTitle(node);
   
   // keep the title, so we can restore it later
   allitems.title = allitems.titlenode.firstChild.data;
   
   // replace the top number with the new value
   allitems.titlenode.firstChild.data = allitems.title.replace(new RegExp(" " + top + " "), " " + count + " ");

   //
   // We may have collapsed a report that is located before the one we are
   // expanding, which would cause the browser to show some arbitrary part
   // of the markup. In order to avoid this, let's tell the browser to go 
   // to the top of the current report. 
   //
   
   // find the anchor element before the report table
   allitems.rptlink = findPrevSibling(findParentNode(node, "table"), "a");

   // and change the document location
   document.location.href = "#" + allitems.rptlink.name;
   
   return true;
}

//
// findHelpTopic
//
// Walks the help container and returns a copy of the node whose title
// attribute matches topic
//
function findHelpTopic(topic)
{
   var node;

   if(helptext) {
      // start with the first child (may be text node)
      node = helptext.firstChild;
   
      // and loop until we found the topic div
      while(node) {
         if(node.nodeType == ELEMENT_NODE && node.title == topic) {
            // clone the node to keep the source remains intact
            node = node.cloneNode(true);
            
            // remove the title attribute, so the browser doesn't show it as a tool tip
            node.removeAttribute("title");
            
            return node;
         }
            
         node = node.nextSibling;
      }
   }
   
   // return a text node if the topic is not found
   return document.createTextNode("Unknown topic (" + topic + ")");
}

//
// showHelp
//
// Copies text from the title node into the help box title div. Also
// locates help definition div by topic and copies it into the help 
// box.
//
//                                      help topic
//                                      v
// <span onclick="showHelp(event, this, topic)">Hits</span>
// ^                              ^              ^
// title node                     title node     title
//
function showHelp(event, titlenode, topic)
{
   var top, left;
   var title = "";
   var titlediv;
   var scrwidth, scrheight;
   var offsetx, offsety;
   
   // nothing to show
   if(!helpbox || !titlenode)
      return;
   
   // get the text from the title node
   titlenode.normalize();
   title = titlenode.firstChild.data;
   
   // and put it in the title div, after the div with x
   titlediv = helpbox.firstChild;
   
   if(titlediv.firstChild.nextSibling)
      titlediv.replaceChild(document.createTextNode(title), titlediv.firstChild.nextSibling);
   else
      titlediv.appendChild(document.createTextNode(title));
   
   // find a help entry by topic and put it after the title div
   if(titlediv.nextSibling)
      helpbox.replaceChild(findHelpTopic(topic), titlediv.nextSibling);
   else
      helpbox.appendChild(findHelpTopic(topic));

   // place the help box around the mouse pointer
   scrwidth = document.body.parentNode.clientWidth;
   scrheight = document.body.parentNode.clientHeight;
   
   offsetx = (event.clientX > scrwidth/2) ? -350 : 20;
   offsety = (event.clientY > scrheight/2) ? -75 : 20;
   
	top = Math.round(document.body.parentNode.scrollTop + event.clientY + offsety);
	left = Math.round(document.body.parentNode.scrollLeft + (event.clientX + offsetx));
	
	helpbox.style.top = top + "px";
	helpbox.style.left = left + "px";

   // and make it visible
   helpbox.style.display = "block";
}

function hideHelp()
{
   if(helpbox.style.display != "none")
      helpbox.style.display = "none";
}

function isHelpShown()
{
   return helpbox.style.display != "none";
}
