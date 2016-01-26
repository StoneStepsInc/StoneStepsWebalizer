/*
   webalizer - a web server log analysis program

   Copyright (c) 2004-2016, Stone Steps Inc. (www.stonesteps.ca)

   See COPYING and Copyright files for additional licensing and copyright information     
   
	webalizer.js
*/

var ELEMENT_NODE  = 1;        // DOM node types
var TEXT_NODE     = 3;

var KEY_ESC       = 27;       // key-up event key codes
var KEY_HOME      = 36;
var KEY_END       = 35;
var KEY_PAGEUP    = 33;
var KEY_PAGEDOWN  = 34;
var KEY_UP        = 38;
var KEY_DOWN      = 40;
var KEY_LEFT      = 37;
var KEY_RIGHT     = 39;

var keyUpHandlers = [];       // functions to call on keyup event

var frags = null;             // report identifiers (fragments)

var helpbox = null;           // help box div
var helptext = null;          // hidden help text container div

//
// Page load event handlers for all three page types 
//
// The caller may pass in a callback function that will be called after all
// standard initialization is done, but before the key-up event handler is 
// installed.
//
function onload_index_page(initCB)
{
   initHelp();

   if(initCB)
      initCB();

   document.onkeyup = onPageKeyUpDispatcher;
}

function onload_usage_page(initCB)
{
   // innitialize fragment navigation
   initPageFragments();

   initHelp();

   if(initCB)
      initCB();

   document.onkeyup = onPageKeyUpDispatcher;
}

function onload_page_all_items(initCB)
{
   if(initCB)
      initCB();

   document.onkeyup = onPageKeyUpDispatcher;
}

//
// Registers a key-up event handler
//
function registerOnKeyUpHandler(handler)
{
   if(handler)
      keyUpHandlers.push(handler);
}

//
// Dispatches page-level key-up events to all registered handlers
//
function onPageKeyUpDispatcher(event)
{
   for(var i = 0; i < keyUpHandlers.length; i++) {
      keyUpHandlers[i](event);
   }

   return true;
}

// ----------------------------------------------------------------------------
//
// Common code
//
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
//
// Fragment navigation
//
// ----------------------------------------------------------------------------

function onKeyUpFragHandler(event)
{
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
}

//
// initPageFragments
//
// Populates the fragment array with fragment identifiers from the 
// main menu
//
function initPageFragments()
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

   registerOnKeyUpHandler(onKeyUpFragHandler);
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

// ----------------------------------------------------------------------------
//
// Help
//
// ----------------------------------------------------------------------------

function onKeyUpHelpHandler(event)
{
   // check if it's the ESC key
   if(event.keyCode == KEY_ESC) {
      // if the help is visible, hide it and return
      if(isHelpShown()) {
         hideHelp();
         return;
      }
   }      
}

function initHelp()
{
   // find the help box div
   if((helpbox = document.getElementById("helpbox")) != null) 
      helpbox.normalize();
   
   // find the help container div
   helptext = document.getElementById("helptext");

   // set up help keyboard handler only if we can show help
   if(helpbox && helptext)
      registerOnKeyUpHandler(onKeyUpHelpHandler);
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
// ^                              ^             ^
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
