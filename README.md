**************************************************************************
Stone Steps Webalizer (v6.1.0)

Copyright (c) 2004-2020, Stone Steps Inc. (www.stonesteps.ca)

The version of The Webalizer provided with this distribution is a fork
based on the version 2.01-10 of the original Webalizer:

The Webalizer - A web server log file analysis tool
Copyright 1997-2000 by Bradford L. Barrett (brad@mrunix.net)

Distributed under the GNU GPL.  See the files "COPYING" and
"Copyright" supplied with the distribution for additional info.

**************************************************************************

## What is The Webalizer?

The Webalizer is a web server log file analysis program which produces
usage statistics in HTML format for viewing with a browser.  The results
are presented in both columnar and graphical format, which facilitates
interpretation.  Yearly, monthly, daily and hourly usage statistics are
presented, along with the ability to display usage by host, URL, referrer,
user agent (browser), search string, entry/exit page, username and country
(some information is only available if supported and present in the log
files being processed).  Processed data may also be exported into most
database and spreadsheet programs that support tab delimited data formats.

The Webalizer supports IIS, Apache (custom format), Squid (HTTP proxy),
CLF (common log format) log files, as well as Combined log formats as
defined by NCSA and others, and variations of these which it attempts to
handle intelligently.

Gzip compressed logs may be used as input directly. Any log filename
that ends with a `.gz` extension will be assumed to be in gzip format and
uncompressed on the fly as it is being read.

In addition, the Webalizer also supports DNS and GeoIP lookup capabilities.

## Installing the Webalizer

### Windows

Windows pre-built package contains all run-time dependencies and can
be used as-is. Extract package contents to any directory, such as
`c:\tools\webalizer\`, and run it from there.

If you intend to use just one configuration file, the installation
directory is probably the best place for it. Otherwise, you can use
the `-c` option to specify any configuration file.

### Linux

Stone Steps Webalizer depends on the following packages:

  * GD Library v2 or newer
  * Berkeley DB v4.3 or newer
  * ZLIB v1 or newer
  * MaxMindDB v1.2 or newer

You can see development package names for some common Linux flavors in
`devops/Docker.*` files in the source repository to figure out binary
packages.

Extract contents of a pre-built Linux package to any directory, such
as `/usr/local/bin/webalizer/`, and run it from there.

### Building from Source

Building from the source on Windows requires Visual Studio 2019 or newer.
All source dependencies are configured as Nuget packages and should be
pulled automatically during a build. Compiled binaries are generated in
the `build` directory.

Building from the source on Linux requires development packages for
all dependencies. See `devops/Docker.*` files for development package
names for some common Linux flavors.

Once all dependencies are installed, change to the source directory 
and run `make`. Compiled binaries are generated in the `build` directory.

## Running the Webalizer

The Webalizer was designed to be run from a Linux or Windows command line
prompt or as a cron job.  There are several command line options which
will modify the results it produces, and configuration files can be used
as well. The format of the command line is:

    webalizer [options ...] [logfile [[ logfile]...] | report-database]

Where `options` can be one or more of the supported command line
switches described below.

`logfile` is the name of the log file to process. Log file names are
collected from three distinct sources:

  * `LogFile` variables in the default configuration file and its
    includes.
  * Command line, configuration files specified with the `-c` option
    and its includes.
  * If the `--pipe-log-names` option was used, log file names are
    read from the standard input.

If a log file name is found in any of the sources above, the previous
set of log file names is cleared, which prevents the possibility of
the same log file name accepted for processing more than once. For
example, if log files `A` and `B` are specified in `webalizer.conf`, and
log files `C` and  `D` are specified on the command line, then only `C`
and `D` will be processed.

Multiple log file collected from the same source will be processed in
their log record time stamp order. This feature is intended for
processing load-balanced log files from multiple web servers behind
the same site, but can also be used to process log files from the same
web server, in which case newer logs will just be processed after older
logs.

If a dash (`-`) is specified for the log-file name, `STDIN` will be used.

`report-database` is the name of the database, not including file path
or file extension, that will be used to generate a report.

Once executed, the general flow of the program follows:

* Any command line arguments given to the program are parsed. This
  may include the specification of a configuration file (`-c`), which
  is processed at the time it is encountered.

* If there was no configuration file specified on the command line
  with the `-c` option, the default configuration file is scanned for.
  The following directories are searched, in the specified order:
  current directory, the system directory (`/etc` on Unix or
  `c:\windows` on Windows), the directory where the executable
  (`webalizer` or `webalizer.exe`) is located.

* If either of the configuration files contained `Include` directives,
  all included configuration files are processed as well.

* If `--prepare-report` was specified on the command line, the last
  argument will be interpreted as a database file name. In this case a
  report will be generated from the data stored in the database and
  no log files will be processed.

* If a log file was specified, it is opened and made ready for
  processing. If no log file was given, or the filename `-` is
  specified on the command line, `STDIN` is used for input.

* If an output directory was specified, the program will generate
  output in this directory. If no output directory was given, the
  current directory is used.

* If a non-zero number of `DNSChildren` processes were specified, the
  corresponding number of DNS worker threads will be started, and IP
  addresses in the specified log file will be either resolved to host
  names or looked up in the GeoIP database, or both.

* If no hostname was given, the program attempts to get the hostname
  using a uname system call.  If that fails, `localhost` is used.

* A history file is searched for.  This file keeps previous month
  totals used on the main index.html page.  The default file is
  named `webalizer.hist`, kept in the specified output directory,
  however may be changed using the `HistoryName` configuration file
  keyword.

* A database file containing the internal state data of the program at
  the end of a previous run is searched for and opened, if found. The
  default database file is `webalizer.db` and is kept in the directory
  specified using the `DbPath` configuration variable.

* Main processing begins on the log file. If the log spans multiple
  months, a separate HTML document is created for each month.

* After main processing, the main `index.html` page is created, which
  has totals by month and links to each months HTML document.

* A new history file is saved to disk, which includes totals generated
  by The Webalizer during the current run.

* The database file is updated to contain the internal state data at the
  end of this run.


## Incremental Processing

Incremental processing preserves current log items and numbers that you
see in all reports, such as URLs, IP addresses, request counts, transfer
amounts, etc., in a database file called webalizer.db and adds new items
to this database or updates numbers for existing items when new log files
are processed.

Originally, incremental processing was an optional mode that made it possible
for website administrators to process multiple rollover log files created by
the web server in the course of one month. The non-incremental mode remained
the default mode and was intended for large log files that contained one or
more months worth of data. With the rapid increase of web traffic over the
years, large log files became hard to manage and most website administrators
choose to rotate their logs either by size or by date. Given this trend, the
incremental mode was changed to be the default processing mode in Stone Steps
Webalizer v4.2.1.

IMPORTANT: Stone Steps Webalizer uses the time stamp in each log record to
track log records that have been already processed and if your logs are rotated
based on the log size, the time stamp in the first few records of each new log
may fall into the same second as last records of the previous log and will be
ignored when that log will be processed. Configure your log rotation based on
the date instead to avoid this, so each new log has distinct log record time
stamps.

Some special precautions need to be taken when using the incremental
run capability of The Webalizer.  Configuration options should not be
changed between runs, as that could cause corruption of the internal
stored data.

For example, changing the `MangleAgents` level will cause different
representations of user agents to be stored, producing invalid results
in the user agents section of the report.  If you need to change
configuration options, do it at the end of the month after normal
processing of the previous month and before processing the current
month. You may also want to delete the database file as well.

The Webalizer also attempts to prevent data duplication by keeping
track of the timestamp of the last record processed.  This timestamp
is then compared to current records being processed, and any records
that were logged previous to that timestamp are ignored.  This, in
theory, should allow you to re-process logs that have already been
processed, or process logs that contain a mix of processed/not yet
processed records, and not produce duplication of statistics.  The
only time this may break is if you have duplicate timestamps in two
separate log files... any records in the second log file that do have
the same timestamp as the last record in the previous log file processed,
will be discarded as if they had already been processed.  This setup
also necessitates that you always process logs in chronological order,
otherwise data loss will occur as a result of the timestamp compare.

## Output Produced

The Webalizer produces several reports (html) and graphics for each
month processed.  In addition, a summary page is generated for the
current and previous months, a history file is created and the current
month's processed data.

The exact location and names of these files can be changed using
configuration files and command line options.  The files produced,
(default names) are:


name                    | description
----                    | -----------
index.html              | Main summary page (extension may be changed)
usage.png               | Yearly graph displayed on the main index page
usage_YYYYMM.html       | Monthly summary page (extension may be changed)
usage_YYYYMM.png        | Monthly usage graph for specified month/year
daily_usage_YYYYMM.png  | Daily usage graph for specified month/year
hourly_usage_YYYYMM.png | Hourly usage graph for specified month/year
site_YYYYMM.html        | All hosts listing (if enabled)
url_YYYYMM.html         | All urls listing (if enabled)
ref_YYYYMM.html         | All referrers listing (if enabled)
agent_YYYYMM.html       | All user agents listing (if enabled)
search_YYYYMM.html      | All search strings listing (if enabled)
webalizer.hist          | Previous month history (may be changed)
webalizer.db            | Incremental Data (may be changed)
webalizer_YYYYMM.db     | Incremental Data (may be changed)
site_YYYYMM.tab         | tab delimited hosts file
url_YYYYMM.tab          | tab delimited urls file
ref_YYYYMM.tab          | tab delimited referrers file
agent_YYYYMM.tab        | tab delimited user agents file
user_YYYYMM.tab         | tab delimited usernames file
search_YYYYMM.tab       | tab delimited search string file

The yearly (index) report shows statistics for a number of months
specified by the HistoryLength configuration parameter and links to
each month.  The monthly report has detailed statistics for that month
with additional links to any URL's and referrers found.
The various totals shown are explained below.

### Hits

  Any request made to the server which is logged, is considered a `hit`.
The requests can be for anything... html pages, graphic images, audio
files, CGI scripts, etc...  Each valid line in the server log is
counted as a hit.  This number represents the total number of requests
that were made to the server during the specified report period. A
request does not have to be successful to be counted as a hit.

For example, if a non-existing file is requested, the web server will
respond with a 404 (Not Found) error and the log file will contain an
entry for this request, which will be counted as one hit.

### Files

  Successful requests served by the server are counted as files. A
file can be an html page or a dynamically processed page, such as a PHP
or ASP page or an image. File total in the reports is a subset of of
hits total.

### Pages

  Pages are, well, pages!  Generally, any HTML document, or anything
that generates an HTML document, would be considered a page.  This
does not include the other stuff that goes into a document, such as
graphic images, audio clips, etc...  This number represents the number
of `pages` requested only, and does not include the other `stuff` that
is in the page.  What actually constitutes a `page` can vary from
server to server.  The default action is to treat anything with the
extension `.htm`, `.html` or `.cgi` as a page.  A lot of sites will
probably define other extensions, such as `.phtml`, `.php3` and `.pl`
as pages as well. Some other programs (and people :) refer to this as
`Pageviews`.

For example, if a request for an HTML document is made that contains
two links to images and one of these images is missing, Stone Steps
Webalizer will count three hits (one for the HTML document and two for
linked image files), two files (one for the HTML document and one for
existing image) and one page (just the HTML document).

### Hosts

  Each request made to the server comes from a unique `host`, which
can be referenced by a name or, ultimately, an IP address.  The
`hosts` number shows how many unique IP addresses made requests to the
server during the reporting time period.  This DOES NOT mean the
number of unique individual users (real people) that visited, which is
impossible to determine using just logs and the HTTP protocol
(however, this number might be about as close as you will get).

### Visits

  Whenever a request is made to the server from a given IP address
(host), the amount of time since a previous request by the address
is calculated.  If the time difference is greater than a
pre-configured `visit timeout` value (or has never made a request before),
it is considered a `new visit`, and this total is incremented for the
host. The default timeout value is 30 minutes (can be changed), so if
a user visits your site at 1:00 in the afternoon, and then returns at
3:00, two visits would be  registered.

Note: in the `Top Hosts` table, the visits total is a sum of all
visits of the grouped hosts.

Note: A visit is started when any request is made to the server,
whether it was successful or not. Consider the following time diagram,
where each `p` represents a successful page request and each `o`
represents any other type of request (e.g. a failed page request, a
file request, etc).

            ~~~~ visit ~~~~                  ~~ visit ~~
           v               v                v           v
     ------o---p------o-o--o----------------o---o-o-----o---------->
           ^~~~^           ^~~~~~~~~~~~~~~~~^                  time
           5 min                 40 min

Stone Steps Webalizer will count in this case two visits, even though
the second visit did not request any pages.

### Transfer

  The Transfer value shows the amount of data that was sent out by
the server during the specified reporting period. This value is
generated directly from the log file, so it is up to the web server
to produce accurate numbers in the logs. In general, this should be
a fairly accurate representation of the amount of traffic the server
had, regardless of the web servers reporting quirks.

By default, most servers only log outgoing amounts (i.e. response
sizes). IIS and Apache may log incoming amounts as well (i.e.
request sizes). Stone Steps Webalizer will include this type of
traffic into the amount reported as Transfer if UpstreamTraffic is
set to `yes` in the configuration file.

Transfer amounts are reported since v4.2 with a unit suffix, such as
`12.3 GB`, or as a number kilobytes. This behavior may be changed by
setting `ClassicKBytes` to `yes`. One kilobyte is counted as either `1024`
or `1000` bytes, depending on the value of the `DecimalKBytes` configuration
variable.

### Top Entry and Exit Pages

  The Top Entry and Exit tables give a rough estimate of what URL's
are used to enter your site, and what the last pages viewed are.
Because of limitations in the HTTP protocol, log rotations, etc...
this number should be considered a good "rough guess" of the actual
numbers, however will give a good indication of the overall trend in
where users come into, and exit, your site.

Sometimes web servers log linked content before logging the page
containing the links. Stone Steps Webalizer will track for each host
whether a page request has been made or not during the current visit
and will report the first page URL, if any, as an entry URL. For
example, if an HTML page contained two linked image files, these files
may be logged before the page itself. Nevertheless, the page URL will
be reported as the entry page.

## Command Line Options

The Webalizer supports many different configuration options that will
alter the way the program behaves and generates output.  Most of these
can be specified on the command line, while some can only be specified
in a configuration file. The command line options are listed below,
with references to the corresponding configuration file keywords.

Note that most command line options are case-sensitive. That is, `-F`
and `-f` are different options.


### General Options

* `-h`, `--help`

    Display all available command line options and exit program.

* `-v`, `-V`, `--version`

    Display program version and exit program.

* `-w`, `-W`, `--warranty`

    Displays the GNU warranty disclaimer.

* `-d`

    Display additional `debugging` information for errors and
    warnings produced during processing.  This normally would
    not be used except to determine why you are getting all those
    errors and wanted to see the actual data.  Normally The
    Webalizer will just tell you it found an error, not the
    actual data.  This option will display the data as well.

    Config file keyword: `Debug`

* `-F`

    Specify that the log file format. By default, Stone Steps
    Webalizer expects IIS log file, but may be instructed to
    process other formats: W3C, IIS, Apache, CLF, Squid.

    Config file keyword: `LogType`

* `-i`

    Ignore history file.  USE WITH CAUTION.  This causes The
    Webalizer to ignore any existing history file produced from
    previous runs and generate it's output from scratch.  The
    effect will be as if The Webalizer is being run for the
    first time and any previous statistics will be lost (although
    the HTML documents, if any, will not be deleted) on the main
    index.html (yearly) web page.

    Config file keyword: `IgnoreHist`

* `-q`

    `Quiet` mode.  Normally, The Webalizer will produce various
    messages while it runs letting you know what it's doing.
    This option will suppress those messages.  It should be
    noted that this WILL NOT suppress errors and warnings, which
    are output to `STDERR`.

    Config file keyword: `Quiet`

* `-Q`

    `ReallyQuiet` mode.  This allows suppression of _all_ messages
    generated by The Webalizer, including warnings and errors.
    Useful when The Webalizer is run as a cron job.

    Config file keyword: `ReallyQuiet`

* `-T`

    Display timing information.  The Webalizer keeps track of the
    time it begins and ends processing, and normally displays the
    total processing time at the end of each run.  If quiet mode
    (`-q` or `Quiet yes` in configuration file) is specified, this
    information is not displayed.  This option forces the display
    of timing totals if quiet mode has been specified, otherwise
    it is redundant and will have no effect.

    Config file keyword: `TimeMe`

* `-c file`

    This option specifies a configuration file to use.  Configuration
    files allow greater control over how The Webalizer behaves, and
    there are several ways to use them. If a configuration file
    is specified with this option, the default configuration
    file will not be processed.

* `-n name`

    This option specifies the hostname for the reports generated.
    The hostname is used in the title of all reports, and is also
    prepended to URL's in the reports. This allows The Webalizer
    to be run on log files for `virtual` web servers or web servers
    that are different than the machine the reports are located on,
    and still allows clicking on the URL's to go to the proper
    location.  If a hostname is not specified, either on the
    command line or in a configuration file, The Webalizer attempts
    to determine the hostname using a `uname` system call.  If this
    fails, `localhost` will be used as the hostname.

    Config file keyword: `SiteName`

* `-o dir`

    This options specifies the output directory for the reports.
    If not specified here or in a configuration file, the current
    default directory will be used for output.

    Config file keyword: `OutputDir`

* `-x name`

    This option allows the generated pages to have an extension
    other than `.html`, which is the default.  Do not include the
    leading period (`.`) when you specify the extension.

    Config file keyword: `HTMLExtension`

* `-P name`

    Specify the file extensions for `pages`.  Pages (sometimes
    called `PageViews`) are normally html documents and CGI
    scripts that display the whole page, not just parts of it.
    Some system will need to define a few more, such as `phtml`,
    `php3` or `pl` in order to have them counted as well.  The
    default is `htm*` and `cgi` for web logs.

    Config file keyword: `PageType`

* `-t name`

    This option specifies the title string for all reports.  This
    string is used, in conjunction with the hostname (if not blank)
    to produce the actual title.  If not specified, the default of
    "Usage Statistics for" will be used.

    Config file keyword: `ReportTitle`

* `-Y`

    Suppress Country graph.  Normally, The Webalizer produces
    country statistics in both Graph and Columnar forms.  This
    option will suppress the Country Graph from being generated.

    Config file keyword: `CountryGraph`

* `-G`

    Suppress hourly graph.  Normally, The Webalizer produces
    hourly statistics in both Graph and Columnar forms.  This
    option will suppress the Hourly Graph only from being generated.

    Config file keyword: `HourlyGraph`

* `-H`

    Suppress Hourly statistics.  Normally, The Webalizer produces
    hourly statistics in both Graph and Columnar forms.  This
    option will suppress the Hourly Statistics table only from
    being generated.

    Config file keyword: `HourlyStats`

* `-L`

    Disable Graph Legends.  The color coded legends displayed on
    the in-line graphs can be disabled with this option.  The
    default is to display the legends.

    Config file keyword: `GraphLegend`

* `-l num`

    Graph Lines.  Specify the number of background reference
    lines displayed on the in-line graphics produced.  The default
    is 2 lines, however can range anywhere from zero (`0`) for
    no lines, up to 20 lines (looks funny!).

    Config file keyword: `GraphLines`

* `-P name`

    Page type.  This is the extension of files you consider to
    be pages for Pages calculations (sometimes called `pageviews`).
    The default is `htm*` and `cgi` (plus whatever HTMLExtension
    you specified if it is different). Don't use a period!

* `-m num`

    Specify a visit timeout. Visits are calculated by looking at
    the time difference between the current and last request made
    by a specific host.  If the difference is greater that the
    visit timeout value, the request is considered a new visit.
    This value is specified in number of seconds.  The default
    is 30 minutes (`1800`). Optional suffixes `m` and `h` may be
    used to specify this value in minutes or hours, respectively.

    Config file keyword: `VisitTimeout`

* `-M num`

    Mangle user agent names. See MangleAgents entry below for
    details about this option.

    Configuration file keyword: `MangleAgents`

* `-g num`

    This option allows you to specify the level of domains name
    grouping to be performed.  The numeric value represents the
    level of grouping, and can be thought of as the "number of
    dots" to be displayed.  The default value of `0` disables any
    domain name grouping.

    Configuration file keyword: `GroupDomains`

* `-D name`

    This allows the specification of a DNS Cache file name.  This
    filename MUST be specified if you have dns lookups enabled
    (using the `-N` command line switch or `DNSChildren` configuration
    keyword).  The filename is relative to the default output
    directory if an absolute path is not specified (ie: starts
    with a leading `/`).

* `-N num`

    Number of DNS child processes to use for reverse DNS and GeoIP
    lookups. If specified, `DNSCache` or `GeoIPDBPath` must also be
    specified. If you do not wish a DNS cache file to be generated,
    omit `DNSCache` to disable DNS resolution. A value `0` will
    disable DNS and GeoIP look-ups.

* `--prepare-report`

    Instructs Stone Steps Webalizer to interpret the last
    argument as a name of the database file rather than a log
    file name and generate monthly report using the data from
    the database.

* `--last-log`

    Allows SSW to avoid generating an unnecessary end-of-month
    report at the beginning of the next month. That is, when a
    log file is being processed, it is not known whether there
    is more data to process or not and, consequently, all active
    visits and downloads are kept active at the end of the run.
    When the first log record from the next month is processed,
    all active visits and downloads are ended and the final
    report is generated. The `--last-month` option allows Stone
    Steps Webalizer to avoid this step by explicitly marking the
    current log file as the last one for the month, so the final
    report can be generated.

* `--batch`

    Instructs Stone Steps Webalizer to run in the batch mode.
    For details, the Database Configuration Options section of
    this document.

* `--end-month`

    End all active visits in the current database, close it and
    roll over the database file. This command works only against
    the current database and requires only the output path. For
    example:

        webalizer -o reports --end-month

* `--compact-db`

    Compact the database file to attempt to decrease its size.

* `--db-info`

    Print information about the specified database

* `--pipe-log-names`

    Instructs Stone Steps Webalizer to read log file names from the
    standard input. Each log name must be on its own line.

    Windows (line break is for readability):

        dir /s /b srv-a\logs\ex1202*.log srv-b\logs\ex1202*.log |
            webalizer -o reports --pipe-log-names

    Linux (line break is for readability):

        ls -1 srv-a/logs/ex1202*.log srv-b/logs/ex1202*.log |
            webalizer -o reports --pipe-log-names

### Hide Options

The following options take a string argument to use as a comparison
for matching.  Except for the IndexAlias, ExcludeSearchArg and
IncludeSearchArg options, the string argument can be plain text, or
plain text that either starts or ends with the wildcard character `*`.

A string argument without an asterisk will be interpreted as a
substring and will match anywhere in the original string. A string
ending with an asterisk will match the beginning of the string. A
string starting with an asterisk will match the end of the string.

Note that asterisks may be specified only at the beginning or end of
the argument and not in the middle. That is, this argument is invalid:
`some*text`. For Example:

Given the string `yourmama/was/here`, the arguments `was`, `*here` and
`your*` will all produce a match.


* `-a name`

    This option allows hiding of user agents (browsers) from the
    "Top User Agents" table in the report.  This option really
    isn't too useful as there are a zillion different names that
    current browsers go by, depending where they were obtained,
    however you might have some particular user agents that hit
    your site a lot that you would like to exclude from the list.
    You must have a web server that includes user agents in it's
    log files for this option to be of any use.  In addition, it
    is also useless if you disable the user agent table in the
    report (see the `-A` command line option or `TopAgents`
    configuration file keyword). You can specify as many of these
    as you want on the command line.  The wildcard character `*`
    can be used either in front of or at the end of the string.
    (ie: `Mozilla/4.0*` would match anything that starts with
    `Mozilla/4.0`).

    Config file keyword: `HideAgent`

* `-r name`

    This option allows hiding of referrers from the "Top Referrer"
    table in the report.  Referrers are URL's, either on your own
    local site or a remote site, that referred the user to a URL
    on your web server.  This option is normally used to hide
    your own server from the table, as your own pages are usually
    the top referrers to your own pages (well, you get the idea).
    You must have a web server that includes referrer information
    in the log files for this option to be of any use.  In addition,
    it is also useless if you disable the referrers table in the
    report (see the `-R` command line option or `TopReferrers`
    configuration file keyword).  You can specify as many of these
    as you like on the command line.

    Config file keyword: `HideReferrer`

* `-s name`

    This option allows hiding of hosts from the "Top Hosts" table
    in the report.  Normally, you will only want to hide your own
    domain name from the report, as it usually is one of the top
    hosts to visit your web server.  This option is of no use if
    you disable the top hosts table in the report (see the -S
    command line option or `TopSites` configuration file option).

    Config file keyword: `HideSite`

* `-X`

    This causes all individual hosts to be hidden, which results
    in only grouped hosts to be displayed on the report.

    Config file keyword: `HideAllHosts`

* `-u name`

    This option allows hiding of URL's from the "Top URL's" table
    in the report.  Normally, this option is used to hide images,
    audio files and other objects your web server dishes out that
    would otherwise clutter up the table.  This option is of no
    use if you disable the top URL's table in the report (see the
    `-U` command line option or `TopURLs` configuration file keyword).

    Config file keyword: `HideURL`

* `-I name`

    This option allows you to specify additional `index.html` aliases.
    Unless `NoDefaultIndexAlias` is specified in the configuration
    file, Stone Steps Webalizer strips the string `index.` from
    URL's before processing, which has the effect of turning a
    URL such as `/somedir/index.html` into just `/somedir/` which is
    really the same URL and should be treated as such.  This
    option allows you to specify _additional_ strings that are
    to be treated the same way.  Use with care, improper use
    could cause unexpected results.

    For example, if you specify the alias string of `home`, a URL
    such as `/somedir/homepages/brad/home.html` would be converted
    into just `/somedir/` which probably isn't what was intended.

    This option is useful if your web server uses a different default
    index page other than the standard `index.html` or `index.htm`,
    such as `home.html` or `homepage.html`.  The string specified
    is searched for _anywhere_ in the URL, so `home.htm` would
    turn both `/somedir/home.htm` and `/somedir/home.html` into
    just `/somedir/`.  Go easy on this one, each string specified
    will be scanned for in EVERY log record, so if you specify a
    bunch of these, you will notice degraded performance.  Wildcards
    are not allowed on this one.

    Config file keyword: `IndexAlias`

### Table Size Options

* `-e num`

    This option specifies the number of entries to display in the
    "Top Entry Pages" table.  To disable the table, use a value of
    zero (`0`).

    Config file keyword: `TopEntry`

* `-E num`

    This option specifies the number of entries to display in the
    "Top Exit Pages" table.  To disable the table, use a value of
    zero (`0`).

    Config file keyword: `TopExit`

* `-A num`

    This option specifies the number of entries to display in the
    "Top User Agents" table.  To disable the table, use a value of
    zero (`0`).

    Config file keyword: `TopAgents`

* `-C num`

    This option specifies the number of entries to display in the
    "Top Countries" table.  To disable the table, use a value of
    zero (`0`).

    Config file keyword: `TopCountries`

* `-R num`

    This option specifies the number of entries to display in the
    "Top Referrers" table.  To disable the table, use a value of
    zero (`0`).

    Config file keyword: `TopReferrers`

* `-S num`

    This option specifies the number of entries to display in the
    "Top Sites" table.  To disable the table, use a value of
    zero (`0`).

    Config file keyword: `TopSites`

* `-U num`

    This option specifies the number of entries to display in the
    "Top URL's" table.  To disable the table, use a value of
    zero (`0`).

    Config file keyword: `TopURLs`

## Configuration Files

The Webalizer allows configuration files to be used in order to simplify
life for all.  There are several ways that configuration files are accessed
by the Webalizer.  When The Webalizer first executes, it looks for a
default configuration file named `webalizer.conf` in the following
directories, in this order:

  * current directory,
  * the system directory (`/etc` on Unix or `c:\windows` on
    Windows),
  * the directory where the executable (`webalizer` or
    `webalizer.exe`) is located.

Alternatively, configuration files may be specified on the command
line with the `-c` option. If the `-c` option is used, the default
configuration file will not be processed.

In addition to the custom and default configuration files, other
configuration files may be processed conditionally using the `Include`
configuration parameter.

For example, the following configuration parameter will instruct
Stone Steps Webalizer to read the configuration file called
`webalizer_hide.conf` located in the specified directory:

    Include    c:\tools\webalizer\webalizer_hide.conf

Configuration files may be included based on the domain name. That is,
if a domain name is specified with the `-n` option, the domain name in
the Include directive will be compared with the command line domain
name.

For example, given these two configuration lines:

    Include c:\tools\webalizer\webalizer_hide-a.conf  www.a.com
    Include c:\tools\webalizer\webalizer_hide-b.conf  www.b.com

, the first include file will be processed if the command line
contains `-n www.a.com` and the second one will be processed if there
is `-n www.b.com`.

Domain-specific includes are particularly useful when processing
log files for multiple sites, as they allow to maintain common
configuration in a single file, which greatly simplifies
configuration maintenance.

There are lots of different ways you can combine the use of
configuration files and command line options to produce various
results. The evaluation order is as follows:

  * Command line arguments are processed. If a configuration file has
    been specified with the `-c` option, it will be processed
    immediately. Options that are further on the command line override
    earlier options, including those that have been processed in the
    configuration file specified with the `-c` option. Any include
    files found in the configuration file are queued for further
    processing.

  * If there was no `-c` option used on the command line, the default
    configuration file is processed. Any include files found in the
    configuration file are queued for further processing.

  * All queued include files are processed in the order they appear in
    the configuration file(s).

If you specify a configuration file on the command line, you
can override options in it by additional command line options which
follow.

For example, most users will most likely want to create the default
file `/etc/webalizer.conf` and place options in it to specify the
hostname, log file, table options, etc.

At the end of the month when a different log file is to be used (the
end of month log), you can run The Webalizer as usual, but put the
different filename on the end of the command line, which will override
the log file specified in the configuration file. It should be noted
that you cannot override some configuration file options by the use
of command line arguments.

For example, if you specify `Quiet yes` in a configuration file, you
cannot override this with a command line argument, as the command line
option only _enables_ the feature (`-q` option).

The configuration files are standard ASCII text files that may be created
or edited using any standard editor.  Blank lines and lines that begin
with a pound sign (`#`) are ignored.  Any other lines are considered to
be configuration lines, and have the form "Keyword Value", where the
`Keyword` is one of the currently available configuration keywords defined
below, and `Value` is the value to assign to that particular option.  Any
text found after the keyword up to the end of the line is considered the
keyword's value, so you should not include anything after the actual value
on the line that is not actually part of the value being assigned.  The
file `sample.conf` provided with the distribution contains lots of useful
documentation and examples as well.  It should be noted that you do not
have to use any configuration files at all, in which case, default values
will be used (which should be sufficient for most sites).

### General Configuration Keywords

* `LogFile`

    This defines the log file to use. It can be a fully qualified
    path or a relative path. In the latter case, the current
    directory and the directory identified with `LogDir` will be
    searched. If used more than one time, Stone Steps Webalizer
    will process log records from each file in their time stamp
    order. If LogFile is not specified and none is provided through
    the command line, the logfile defaults to `STDIN`.

* `LogDir`

    Defines an optional path to the log directory. If `LogDir` is not
    empty and LogFile is not an absolute path (i.e. not starting with
    a drive letter or a path separator slash character), the complete
    log file path will be derived by combining LogDir and LogFile.

    Default value: none

* `LogType`

    This specified the log file type being used. Values may
    be either `w3c`, `iis`, `apache`, `clf` or `squid`.
    Ensure that you specify the proper file type, otherwise
    you will be presented with a long stream of `invalid
    record` messages ;)

    Command line argument: `-F`

* `OutputDir`

    This defines the output directory to use for the reports.  If
    it is not specified, the current directory is used.

    Command line argument: `-o`

* `OutputFormat`

    Specifies the format of the generated reports. These formats,
    are supported:

     * HTML
     * TSV

    HTML is the default format and will be used if no other format
    is specified.

    TSV stands for "tab-separated values" and will instruct Stone
    Steps Webalizer to generate .tab files, as if all `DumpX` options
    were set to `yes`. Note that if at least one `DumpX` option is used,
    TSV report is added automatically to the list of output formats.

    Multiple `OutputFormat` entries may be used in order to generate
    reports in more than one format.

* `HistoryName`

    Allows specification of a history path/filename if desired.
    The default is to use the file named `webalizer.hist`, kept
    in the normal output directory (`OutputDir` above).  Any name
    specified is relative to the normal output directory unless
    an absolute path name is given (ie: starts with a `/`).

* `ReportTitle`

    This specifies the title to use for the generated reports.
    It is used in conjunction with the hostname (unless blank)
    to produce the final report titles.  If not defined, the
    default of "Usage Statistics for" is used.

    Command line argument: `-t`

* `SiteName`

    This defines the site hostname. The hostname is used in the
    report title as well as being prepended to URL's in the
    "Top URL's" table.  This allows The Webalizer to be run
    on "virtual" web servers, or servers that do not reside
    on the local machine, and allows clicking on the URL to
    go to the right place.  If not specified, The Webalizer
    attempts to get the hostname via a `uname` system call,
    and if that fails, will default to `localhost`.

    Command line argument: `-n`

* `SiteAlias`

    Specifies an alternative site host name. Multiple `SiteAlias`
    entries can be used in configuration files to register more
    than one alias. Site aliases are used as a white list when
    checking for spam links in search strings.

* `UseHTTPS`

    Causes the links in the `Top URL's` table to use `https://`
    instead of the default `http://` prefix if the request port
    is either not present in the logs or if the URL was requested
    over a secure port at least once. Ports are identified by
    `HttpsPort` and `HttpPort` settings, respectively. `UseHTTPS` is
    ignored for URLs that were requested over the port identified
    by `HttpPort` or some port other than `HttpsPort`.

    For example, if `HttpPort` is `80` and `HttpsPort` is `443` and

    * page `A` was requested over port `80`,
    * page `B` was requested over port `443`,
    * page `C` was requested over ports `80` and `443`,
    * page `D` was requested over ports `80` and `8080`,
    * page `E` was requested over ports `8080` and `443`

    , then when `UseHTTPS` is set to `no`, only page `B` will be rendered
    with the `https://` prefix and when `UseHTTPS` is set to `yes`,
    pages `B`, `C` and `E` will be rendered with the `https://` prefix.

* `Quiet`

    This allows you to enable or disable informational messages
    while it is running.  The values for this keyword can be
    either `yes` or `no`.  Using "Quiet yes" will suppress these
    messages, while "Quiet no" will enable them.  The default
    is `no` if not specified, which will allow The Webalizer
    to display informational messages.  It should be noted that
    this option has no effect on Warning or Error messages that
    may be generated, as they go to `STDERR`.

    Command line argument: `-q`

* `TimeMe`

    This allows you to display timing information regardless of
    any "quiet mode" specified.  Useful only if you did in fact
    tell the webalizer to be quiet either by using the `-q` command
    line option or the `Quiet` keyword, otherwise timing stats
    are normally displayed anyway.  Values may be either `yes`
    or `no`, with the default being `no`.

    Command line argument: `-T`

* `UTCTime`, `GMTTime`

    This keyword allows timestamps to be displayed in GMT (UTC)
    time instead of local time.  Normally The Webalizer will
    display timestamps in the time-zone of the local machine
    (ie: PST or EDT).  This keyword allows you to specify the
    display of timestamps in GMT (UTC) time instead.  Values
    may be either `yes` or `no`.

    Default is `no`.

* `UTCOffset`

    Specifies the difference between the local time and UTC time,
    without factoring in the daylight saving time adjustment. For
    example, Eastern Standard Time (EST) is 5 hours behind UTC and
    the `UTCOffset` value for EST would be -5h. `UTCOffset` is only
    applied when the log time zone type doesn't match the UTCTime
    value. In other words, log time in Apache and CLF logs will
    not be adjusted by `UTCOffset` if `UTCTime` is set to `no`, but
    IIS logs will be in the same configuration.

    Default value: `0`

* `LocalUTCOffset`

    Controls if the UTC offset of the machine running Stone
    Steps Webalizer can be used to automatically set `UTCOffset`.
    Note that daylight savings time offset is not included into
    `UTCOffset`.

    Default value: `no`

* `DSTOffset`

    Specifies the difference between the standard and daylight
    saving time. For example, setting `DSTOffset` to 1h instructs
    Stone Steps Webalizer to add one hour to those log time
    stamps that are greater than or equal to `DSTStart` and less
    than `DSTEnd`.

    Default value: `0`

* `DSTStart`, `DSTEnd`

    Specifies the beginning and the end of the daylight saving
    time. `DSTStart` is in local time and `DSTEnd` is in local
    daylight saving time. All log time stamps greater than or
    equal to `DSTStart` and less than `DSTEnd` will be adjusted by
    `DSTOffset`.

    Multiple `DSTStart` and `DSTEnd` ranges may be added to cover more
    than one year. For example, this configures Eastern Daylight
    Saving Time for `2009` and `2010`:

        DSTStart		2009/03/09 2:00
        DSTEnd		2009/11/01 2:00
        DSTStart		2010/03/14 2:00
        DSTEnd		2010/11/07 2:00

    `DSTStart` and `DSTEnd` are not evaluated if `DSTOffset` is zero.

    Default value: none

* `Debug`

    This tells The Webalizer to display additional information
    when it encounters Warnings or Errors.  Normally, The
    Webalizer will just tell you it found a bad record or
    field.  This option will enable the display of the actual
    data that produced the Warning or Error as well.  Useful
    only if you start getting lots of Warnings or Errors and
    want to determine the cause.  Values may be either `yes`
    or `no`, with the default being `no`.

    Command line argument: `-d`

* `IgnoreHist`

    This suppresses the reading of a history file.  USE WITH
    EXTREME CAUTION as the history file is how The Webalizer
    keeps track of previous months.  The effect of this option
    is as if The Webalizer was being run for the very first
    time, and any previous data is discarded.  Values may be
    either `yes` or `no`, with the default being `no`.

    Command line argument: `-i`

* `VisitTimeout`

    Set the `visit timeout` value.  Visits are determined by
    looking at the time difference between the current and last
    request made by a specific host.  If the difference in time
    is greater than the visit timeout value, the request is
    considered a new visit.  The value is in number of seconds,
    and defaults to 30 minutes (`1800`). Optional suffixes `m` and
    `h` may be used to specify this value in minutes or hours,
    respectively.

    Command line argument: `-m`

* `PageType`

    Allows you to define the `page` type extension.  Normally,
    people consider HTML and CGI scripts as `pages`.  This
    option allows you to specify what extensions you consider
    a page.  Default is `txt`, `php`, `htm*` and `cgi` for
    Apache and CLF logs, `txt`, `asp`, `aspx`, and `htm*`
    for IIS logs.

    Command line argument: `-P`

* `PageEntryURL`

    HTTP requests are logged at the end of the request processing
    cycle, which may cause a page to be logged after all of the
    resources it references, such as images or style sheets. In
    this case, an entry URL may be an image or some other file,
    which is not very informative. When `PageEntryURL` is set to
    `yes`, Stone Steps Webalizer will ignore non-pages when
    collecting data for entry URL. If set to `no`, any successful
    first request in a visit will be reported as an entry URL,
    regardless of its type.

    Default value: `yes`

* `GraphLegend`

    Enable/disable the display of color coded legends on the
    produced graphs.  Default is `yes`, to display them.

    Command line argument: `-L`

* `GraphLines`

    Specify the number of background reference lines to display
    on produced graphs.  The default is `2`.  To disable the use
    of background lines, use zero (`0`).

    Command line argument: `-l`

* `CountryGraph`

    This keyword is used to either enable or disable the creation
    and display of the Country Usage graph.  Values may be either
`yes` or `no`, with the default being `yes`.

    Command line argument: `-Y`

* `JavaScriptChartsMap`

    If `CountryGraph` is enabled and `JavaScriptCharts` is configured,
    the country chart will be rendered as a world map instead of
    a pie chart.

* `DailyGraph`

    This keyword is used to either enable or disable the creation
    and display of the Daily Usage graph.  Values may be either
`yes` or `no`, with the default being `yes`.

* `DailyStats`

    This keyword is used to either enable or disable the creation
    and display of the Daily Usage statistics table.  Values may
    be either `yes` or `no`, with the default being `yes`.

* `HourlyGraph`

    This keyword is used to either enable or disable the creation
    and display of the Hourly Usage graph.  Values may be either
`yes` or `no`, with the default being `yes`.

    Command line argument: `-G`

* `HourlyStats`

    This keyword is used to either enable or disable the creation
    and display of the Hourly Usage statistics table.  Values may
    be either `yes` or `no`, with the default being `yes`.

    Command line argument: `-H`

* `IndexAlias`

    This allows additional `index.html` aliases to be defined.
    Normally, The Webalizer scans for and strips the string
    "index." from URL's before processing them.  This turns a
    URL such as `/somedir/index.html` into just `/somedir/` which
    is really the same URL.  This keyword allows _additional_
    names to be treated in the same fashion for sites that use
    different default names, such as `home.html`.  The string
    is scanned for anywhere in the URL, so care should be used
    if and when you define additional aliases.

    For example, if you were to use an alias such as `home`, the
    URL `/somedir/homepages/brad/home.html` would be turned into
    just `/somedir/` which probably isn't the intended result.
    Instead, you should have specified `home.htm` which would
    correctly turn the URL into `/somedir/homepages/brad/` like
    intended.

    It should also be noted that specified aliases are scanned
    for in EVERY log record... A bunch of aliases will noticeably
    degrade performance as each record has to be scanned for
    every alias defined.  You don't have to specify `index.` as
    it is always the default.

    Command line argument: `-I`

* `MangleAgents`

    The `MangleAgents` keyword specifies the level of user agent
    name mangling, if any.

    Normally, The Webalizer will keep track of the user agent field
    verbatim. Unfortunately, there are a ton of different names that
    user agents go by, and the field also reports other items such as
    machine type and OS used.

    For example, Netscape 4.03 running on Windows 95 will report a
    different string than Netscape 4.03 running on Windows NT, so
    even though they are the same browser type, they will be considered
    as two totally different browsers by The Webalizer. For that matter,
    Netscape 4.0 running on Windows NT will report different names if
    one is run on an Alpha and the other on an Intel processor!
    Internet Exploder is even worse, as it reports itself as if it
    were Netscape and you have to search the given string a little
    deeper to discover that it is really MSIE! In order to consolidate
    generic browser types, this option will cause The Webalizer to
    "mangle" the user agent field, attempting to consolidate generic
    browser types.

    Stone Steps Webalizer has two methods for mangling user agent
    names. One is the classic Webalizer method and one is a new
    method based on user agent filters (`ExcludeAgentArgs`,
    `IncludeAgentArgs` and `GroupAgentArgs`).

    **Classic User Agent Name Mangling**

    Classic user agent mangling method allows to specify 5 levels
    of mangling, each producing different level of detail.

    * Level 5 displays only the browser name (MSIE or Mozilla) and
      the major version number.

    * Level 4 will also display the minor version number (single
      decimal place).

    * Level 3 will display the minor version number to two decimal
      places.

    * Level 2 will add any sub-level designation (such as Mozilla/3.01Gold
      or MSIE 3.0b).

    * Level 1 will also attempt to add the system type.

    The default Level 0 will disable name mangling and leave the
    user agent field unmodified, producing the greatest amount of
    detail.

    **User Agent Name Mangling Filters**

    If `UseClassicMangleAgents` is set to `yes`, filter-based mangling
    method will be used instead, in which case if exclude and group
    agent argument lists are empty and `MangleAgents` is set to a
    non-zero value, pre-defined exclude and group filters will be
    added for each of the 4 supported levels of `MangleAgents`.

    When user agent arguments are being processed, Stone Steps
    Webalizer recognizes product versions, which are expressed
    as `product/x.y.z`, where `x.y.z` is the product version. Version
    number will be truncated by different levels of `MangleAgents`.

    Note that some products (most notably Internet Explorer) do
    not follow HTTP RFC and do not separate version numbers from
    the product using a slash character. As a result, the current
    implementation of Stone Steps Webalizer does not truncate
    these version numbers as described below.

    * Level 1 will filter out overused and cryptic strings, such as
      `.NET CLR x.y.z` or `Mozilla/5.0`. The former has little value
      for analysis and the latter is overused to the point when its
      presence or format is far from the truth (e.g. IE identifies
      itself as `Mozilla/4.0`).

    * Level 2 will filter out less-known component identifiers,
      such as Gecko (Mozilla's HTML rendering engine), and Mozilla's
      security level identifiers (i.e. U, I and E). At this level,
      product version numbers are truncated to include only the minor
      version.

    * Level 3 will filter out the word Windows, which is used by
      Mozilla browsers to identify Windows platform, in addition to
      reporting Windows version, such as Windows NT 5.1, and name
      popular Windows versions (e.g. Windows NT 5.1 as Windows XP).
      Version numbers are truncated to the major version number at
      this level.

    * Level 4 will filter out many platform and CPU identifiers,
      such as `Red Hat/1.2.3.4` or `PPC Mac OS X`. Version numbers
      are truncated to include only the product name at this level.

    Note that if exclude or group agent argument lists are not
    empty, the default filters described above will not be added.
    However, version truncation will still be performed, as
    described for each level above.

    Command line argument: `-M`

* `SearchEngine`

    This keyword allows specification of search engines and
    their query strings.  Search strings are obtained from
    the referrer field in the record, and in order to work
    properly, the Webalizer needs to know what query strings
    different search engines use.  The SearchEngine allows
    you to specify the search engine and it's query string
    to parse the search string from.  The line is formatted
    as:  "SearchEngine engine-string query-string"  where
    `engine-string` is a substring for matching the search
    engine with, such as "yahoo.com" or `altavista`.  The
    `query-string` is the unique query string that is added
    to the URL for the search engine, such as `search=` or
    `MT=` with the actual search strings appended to the
    end.  There is no command line option for this keyword.

    This configuration parameter supports additional syntax
    to be able to combine various search terms.

    For example, somebody using Google to find pages that
    contain all words and to be a certain file type (e.g. PDF)
    use different search arguments compared to a usual search.
    Following configuration allows Stone Steps Webalizer to
    process such cases using this configuration:

        SearchEngine    www.google.     as_q=All Words
        SearchEngine    www.google.     as_filetype=File Type

    All matching search strings will be reported on one
    line, separated by the term qualifier.

    For example, the following line describes that somebody
    was looking for a PDF file containing words `webalizer`
    and `apache`:

        [All Words] webalizer apache [File Type] pdf

    For performance reasons, all search terms for the same
    site (e.g. `www.google.`) must be grouped. The first line
    with the mismatching domain name pattern will cancel
    further search.

* `Incremental`

    This allows incremental processing to be enabled or disabled.
    Incremental processing allows processing partial logs without
    the loss of detail data from previous runs in the same month.
    This feature saves the `internal state` of the program so that
    it may be restored in following runs.  See the section above
    titled "Incremental Processing" for additional information.
    The value may be `yes` or `no`, with the default being `yes`.

* `ApacheLogFormat`

    Defines the format Stone Steps Webalizer will use when
    parsing custom Apache log files. This configuration
    parameter will only be evaluated when the current log file
    type is Apache. Search this document for CustomLog for more
    details about Apache custom log format.

    Default value: none

* `BundleGroups`

    Controls whether grouped items in the reports should be
    bundled together at the beginning of the report or not.
    Bundling groups together makes it easier to stack them up
    against each other.

    Default value: `yes`

* `ConvURLsLowerCase`

    Controls whether URL characters will be converted to lower
    case (`yes`) or not (`no`). This option is evaluated only when
    Stone Steps Webalizer is processing IIS or Apache custom log
    files.

    Default value: `no`

* `DownloadPath`

    Lists a URL path that Stone Steps Webalizer will use to
    detect file downloads for the downloads report.

    For example, if you would like to track downloads of a file
    called `util.zip` located in the `/downloads/` directory,
    add the following entry to `webalizer.conf`:

        DownloadPath    /downloads/util.zip    Utility Download

    Wildcard characters (*) may be used at the beginning and the
    end of the path to list partial paths. Note that query
    strings are ignored when logged URL's are compared with
    `DownloadPath` entries. Multiple `DownloadPath` entries may be
    used to track more than one path.

    Default value: none

* `DownloadTimeout`

    Maximum number of seconds between consecutive partial
    download requests that are counted towards the same download
    job.

    Default value: `180`

* `EnablePhraseValues`

    If this configuration parameter is set to yes, Stone Steps
    Webalizer will treat the tab character as a name/value
    separator when parsing two-part configuration entries, such
    as `GroupAgent` or `SearchEngine` and will ignore spaces
    embedded in the values. For example:

        Mozilla/4.0 (compatible; MSIE 6* <tab> Internet Explorer v6
        ^----------- value ------------^       ^------ name ------^

    Default value: `no`

* `GroupURLDomains`

    Squid log files contain absolute URLs, along with fully-
    qualified domain names. GroupURLDomains may be used to group
    these domains in the URL report. The value of this
    configuration parameter is the number of domain labels, past
    the top-level one, to report.

    For example, if `GroupURLDomains` is set to `1`, two labels will
    be reported (e.g. `stonesteps.ca`); if this parameter is set to
    `2`, three labels will be reported (e.g. `forums.stonesteps.ca`);
    and so on. If `GroupURLDomains` is set to `0`, no this type of
    grouping will not be performed.

    Default value: `0`

* `HistoryLength`

    Defines the maximum number of months reported on the main
    index page. The minimum number of months in the history is
    `12`.

    Default value: `24`

* `HttpPort`

    Defines the TCP/IP port used by the web server to serve HTTP
    requests.

    Default value: `80`

* `HttpsPort`

    Defines the TCP/IP port used by the web server to serve
    HTTPS requests.

    Default value: `443`

* `Include`

    Instructs Stone Steps Webalizer to process the specified
    configuration file after the main configuration file has
    been processed. This parameter may optionally be followed by
    the domain name to make the include directive domain
    specific.

    For example, the following include directive will only be
    processed if the domain name specified with the `-n` option
    is `www.a.com`.

        Include   c:\tools\webalizer\a.conf    www.a.com

    Default value: none

* `IncludeSearchArg`, `ExcludeSearchArg`

    Define include and exclude search arguments filters. Each
    configuration parameter is expected to be either a complete
    or a partial name of a search argument to include or
    exclude. A single asterisk may be used to include or exclude
    all search arguments. Multiple include/exclude directives
    may be used if more than one search argument is to be
    included or excluded. The include filter takes precedence
    over the exclude filter.

    Unlike other include/ignore filters, non-wildcard Include-
    and ExcludeSearchArg values are not treated as sub-strings
    and must match search argument names exactly in order for
    the corresponding filter to be activated.

    For example, the following two exclude filters will remove
    search arguments `x` and `y`, which are commonly submitted by
    browsers if image-based buttons are used on the page, but
    will not affect search arguments that contain characters `x`
    or `y`, such as query or xpath.

        ExcludeSearchArg   x
        ExcludeSearchArg   y

    Default value: none

* `IncludeAgentArgs`, `ExcludeAgentArgs`

    These filters are implemented in the same way as are include
    and exclude search argument filters, except that they are
    applied to the user agent name.

* `GroupAgentArgs`

    This filter makes it possible to rename matching user agent
    arguments. For example, this filter replaces "Windows NT 5.1"
    with "Windows XP" in the final output:

        GroupAgentArgs   Windows NT 5.1  Windows XP

    Note that if the pattern contains spaces (e.g. Windows NT 5.1),
    there must be a tab character between the pattern and the alias
    and EnablePhraseValues must be set to `yes`. Otherwise, the
    first space will end the pattern (in this example, the pattern
    will end after the first word Windows).

* `LanguageFile`

    Specifies a fully-qualified path to the language file.

* `MonthlyTotals`

    Specifies whether Stone Steps Webalizer should generate the
    monthly totals report or not.

    Default value: `yes`

* `NoDefaultIndexAlias`

    Many web servers make it possible to configure a default
    document for each directory. If a user requests a URL that
    is a directory (e.g. `http://127.0.0.1/books/`), the default
    document from that directory will be served.

    For example, IIS is usually configured with default.htm as a
    default document; Apache, as well as many other Unix-originated
    web servers, is configured to serve index.html as a default
    document. Stone Steps Webalizer, by default, adds index. to
    the list of default documents. When processing a URL, Stone
    Steps Webalizer checks if the requested file matches any
    entries in the default document list and if it does, strips
    off the file name from the URL.

    This feature allows Stone Steps Webalizer to avoid
    fragmenting default document statistics if the same document
    was requested using multiple aliases (e.g. `index.html`,
    `index.php`, etc). However, in some cases, it is undesirable
    to use index. as a default alias (e.g. if there is a
    directory named `index.ext`). Setting `NoDefaultIndexAlias` to
    `yes` prevents Stone Steps Webalizer from adding index. to the
    default document list.

    Default value: `no`

* `SortSearchArgs`

    Controls whether search arguments will be sorted
    alphabetically or not. Sorting search arguments helps
    defragmenting URL reports.

    Default value: `yes`

* `SpamReferrer`

    Each entry lists a keyword identifying the visitor as a
    spammer. Multiple values may be used to specify more than
    one keyword. Once identified as a spammer, visitor's IP
    address will be remembered for the rest of the month and any
    requests originating from this IP address will be treated as
    spam.

    Default value: none

* `UpstreamTraffic`

    Indicates whether to track upstream data transfers (i.e.
    uploads) or not. Note that upstream and downstream transfer
    amounts are not tracked separately - their values are added
    together and shown as Transfer.

    Default value: `no`

* `Robot`

    Defines a pattern used to identify robot user agents, such
    as search engines. The pattern may contain a leading or a
    trailing wildcard character (`*`) to indicate that the pattern
    should be matched from the end or from the beginnning of
    the string, respectively. Otherwise, the pattern will be
    treated as a sub-string. Leading wildcards are not very
    useful for filtering user agents.

    For example, `msnbot/*` will match the first user agent and
    `Googlebot/` will match the second one.

        msnbot/1.0 ( http://search.msn.com/msnbot.htm)
        Mozilla/5.0 (compatible; Googlebot/2.1;)

    Robot entries may contain aliases that will be used instead
    of patterns when grouping robot entries. For example:

        Robot   Mediapartners-Google*     Google Adsense
        Robot   msnbot*                   MSN Live Search

    Default value: none

* `TargetURL`

    Visitors who browse a website and then purchase, download
    something or just visit a designated page are called
    converted visitors. Converted visitors may be tracked by
    specifying target URL patterns in the configuration file.
    Each `TargetURL` entry designates a URL pattern. For example,

        TargetURL    /orders/receipt.asp*

    This entry instructs Stone Steps Webalizer to interpret
    the URL that begins with `/orders/receipt.asp` to be treated
    as a target URL.

    Multiple TargetURL entries can be used to specify more
    than one pattern.

    Default value: none

* `TargetDownload`

    If downloads are being tracked using `DownloadPath`, setting
    `TargetDownload` to `yes` will instruct Stone Steps Webalizer
    to treat those requests that matched DownloadPath as if they
    had a corresponding TargetURL entry. This improves overall
    performance by reducing the number of target URL entries and
    simplifies configuration.

    Default value: `yes`

* `ClassicKBytes`

    Starting with v4.2, Stone Steps Webalizer shows transfer amounts in
    reports and charts as numbers with a unit suffix, such as KB, MB, GB,
    etc., as opposed to the traditional numbers of kilobytes. If you would
    like to revert to the previous behavior, set `ClassicKBytes` to `yes` in
    the configuration. You will also need to change the word Transfer to
    KBytes in the language file, so report titles look the same as before
    v4.2.

    Default value: `no`

* `DecimalKBytes`

    Traditionally, the Webalizer interpreted one kilobyte as 1024 bytes.
    You can set DecimalKBytes to `yes` to compute transfer amounts in
    multiples of 1000 bytes.

    Default value: `no`

* `PageTitle`

    Associates a page title with a URL pattern. If a URL in the
    Top N URLs and Entry/Exit URL reports match some pattern,
    the pattern name will be rendered instead of the URL. For
    example, blog pages might be set up like this:

        PageTitle     /post/12*   Wild Life Photography
        PageTitle     /post/34*   Action Photography

    In the All URLs report the matching pattern name will not
    be rendered instead of the URL text, but instead will be
    added as a URL title, so hovering over that entry will show
    the title. This way URLs are shown more consistently in
    a tabular report with a lot of other URLs that are not
    titled.

    The pattern syntax is the same as for hide/ignore options and
    if one is specified without an asterisk, it will match any
    text within the URL. Trailing asterisk, like that shown above,
    will match URLs from the beginning. Note, however, that the
    order of entries matches and longer ones should be listed
    first.

    For example, an entry with an ID `1` and a trailing asterisk
    would mask the following ID `18` and the latter would never
    appear in the report:

        PageTitle     /post/1*    Wild Life Photography
        PageTitle     /post/18*   Action Photography

    Reordering `18*` before `1*` will assign correct titles in
    the report, but will `1*` will still match other sequences
    starting with `1` if they are not explicitly listed as page
    titles (e.g. `/post/19`). Alternatively, you can use a leading
    asterisk, which will work for cases above, but may misfire
    for unrelated URLs ending with the same sequence, such as
    login redirects (e.g. `/login/?ref=/post/19` will be titled
    with the pattern `*/post/19`).

    Note that without an asterisk the pattern will be match any
    emedded text, such as `/post/1` in a URL `/?ref=/post/123`.

    URL patterns are matched against normalized URLs, so if you
    want to match some non-ASCII character, use the actual
    character and not a URL-encoded sequence for that character.
    See the URL Normalization section for details.

    Page titles can be styled in `webalizer.css`. By default
    report table cells containing page titles are styled to
    insert a Unicode page character before title text.

### DNS Resolution Configuration Options

* `DNSCache`

    Specifies the DNS cache filename.  This name is relative
    to the default output directory unless an absolute name
    is given (ie: starts with `/`). If `DNSCache` is not
    specified, but `GeoIPDBPath` is, DNS resolver workers will
    still be created to lookup country codes during log file
    processing.

* `ASNDBPath`

    A fully-qualified path to a MaxMind's Autonomous System
    numbers (ASN) database. A free ASN database may be downloaded
    from this location:

    https://dev.maxmind.com/geoip/geoip2/geolite2/

    When ASN database is configured, ASN columns will be added
    to the Hosts and Downloads reports and an additional Top ASN
    report will be generated, as well as a tab-separated file
    containing ASN entries. Use TopASN and `DumpASN` configuration
    parameters to control whether to generate a top ASN report
    and to generate a tab-separated ASN file.

* `GeoIPDBPath`

    This configuration parameter is expected to be a fully-
    qualified path to the MaxMind's GeoIP Country or GeoIP
    City database file in the binary format. The database may
    be downloaded at the following URL:

    http://www.maxmind.com/app/geoip_country

    Less precise, but free GeoIP databases are available at
    this location:

    http://dev.maxmind.com/geoip/geoip2/geolite2/

    Setting `GeoIPDBPath` will instruct Stone Steps Webalizer
    to use the information in the GeoIP database to generate
    the country report. If this parameter is not set or if
    it points to a non-existent or invalid GeoIP database,
    Stone Steps Webalizer will use domain name suffixes,
    such as `.ca` or `.jp`, to generate the country report.

* `GeoIPCity`

    Value `yes` will instruct Stone Steps Webalizer to look up
    country and city names in the GeoIP database, while value
    `no` will disable city look-ups. Note that you need GeoIP
    City database for this parameter to have any effect.

    Default value: `yes`

* `DNSChildren`

    Number of DNS child processes to use for reverse DNS and GeoIP
    lookups. If specified, `DNSCache` or `GeoIPDBPath` must also be
    specified. If you do not wish a DNS cache file to be generated,
    omit DNSCache to disable DNS resolution. A value `0` will
    disable DNS and GeoIP look-ups.

* `DNSCacheTTL`

    Specifies Time To Live (TTL) in days for DNS cache entries.
    In most cases it is reasonable to set this value to 30 days.

    Default value: `30`

* `DNSLookups`

    Specifies whether to resolve host IP addresses to domain
    names (yes) or not (no) if `DNSCache` and `DNSChildren` are
    configured to enable DNS components. When set to `no`,
    DNS resolution is disabled, but the `DNSCache` database is
    still used to preserve some IP address information, such
    as whether some IP address is marked as a spammer, from
    month to month.

    Default: `yes`

* `AcceptHostNames`

    Specifies whether to accept host names instead of IP addresses
    in log files or not. Configuring your web server to resolve
    IP addresses to host names will slow down the server. Lack
    of IP addresses also will disable address-based visitor
    country identification.

    Default value: `no`

* `ExternalMapURL`

    Specifies a fully-qualified URL of any map service that can
    interpret latitude and longitude expressed as signed decimal
    degrees. Use `{{lat}}` and `{{lon}}` to pass coordinates to the
    map service. For example, use this URL to show a map centered
    on the location of the IP address in Google Maps:

    https://www.google.ca/maps/@{{lat}},{{lon}},12z

### Graph Configuration

* `GraphBackgroundAlpha`

    Sets the transparency of the background of graph images, in
    percent. The value of `100` makes background completely
    transparent, while the value of 0 makes it completely
    opaque. Making graph backgrounds transparent makes it
    possible to use another image, such as a logo, a gradient or
    a pattern as a graph background. `GraphTrueColor` must be set
    to yes in order for `GraphBackgroundAlpha` to work.

    Default value: `0`

* `GraphBorderWidth`

    Defines the width of the 3D border around image graphs in
    pixels. If set to zero, graph images are generated without a
    border. This value cannot be greater than `8`.

    Default value: `0`

* `GraphFontNormal`, `GraphFontBold`

    Define fully-qualified paths to TrueType font files that
    Stone Steps Webalizer will use when creating graphs. If
    these paths are not specified, Stone Steps Webalizer will
    use default raster fonts.

    Default value: none

* `GraphBackgroundColor`

    Defines the background color for all graphs generated by
    Stone Steps Webalizer. The value must be specified as six
    hexadecimal digits, two for each color - red, green and
    blue.

    Default value: `C0C0C0`

* `GraphFontMedium`

    Specifies the size, in points, of the small and medium fonts
    used in graphs. The medium font is used for graph titles and
    country names in the country report.

    Default value: `9.5`

* `GraphFontSmall`

    Specifies the size, in points, of the small font used in
    graphs. The small font is used for graph legends (e.g. Hits,
    Visits, etc) and axis markers.

    Default value: `8`

* `GraphFontSmoothing`

    Specifies whether Stone Steps Webalizer will create graphs
    using smoothed TrueType fonts. This value is ignored if
    default raster fonts are used.

    Default value: `yes`

* `GraphGridlineColor`

    Defines the color of graph gridlines. The value must be
    specified as six hexadecimal digits, two for each color -
    red, green and blue.

    Default value: `808080`

* `GraphHitsColor`

    Defines the color of the graph and legend associated with
    Hits. The value must be specified as six hexadecimal digits,
    two for each color - red, green and blue.

    Default value: `00805C`

* `GraphHostsColor`

    Defines the color of the graph and legend associated with
    Hosts. The value must be specified as six hexadecimal
    digits, two for each color - red, green and blue.

    Default value: `FF8000`

* `GraphLegendColor`

    Defines the base color of the X-axis legend. The value must
    be specified as six hexadecimal digits, two for each color -
    red, green and blue.

    Default value: `000000`

* `GraphOutlineColor`

    Defines the color of the graph bar outlines. The value must
    be specified as six hexadecimal digits, two for each color -
    red, green and blue.

    Default value: `000000`

* `GraphPagesColor`

    Defines the color of the graph and legend associated with
    Pages. The value must be specified as six hexadecimal
    digits, two for each color - red, green and blue.

    Default value: `00C0FF`

* `GraphTitleColor`

    Defines the color of the graph title. The value must be
    specified as six hexadecimal digits, two for each color -
    red, green and blue.

    Default value: `0000FF`

* `GraphVisitsColor`

    Defines the color of the graph and legend associated with
    Visits. The value must be specified as six hexadecimal
    digits, two for each color - red, green and blue.

    Default value: `FFFF00`

* `GraphTransferColor`

    Defines the color of the graph and legend associated with
    Transfer. The value must be specified as six hexadecimal
    digits, two for each color - red, green and blue.

    Default value: `FF0000`

* `GraphWeekendColor`

    Defines the color of the weekend days in the monthly traffic
    report. The value must be specified as six hexadecimal
    digits, two for each color - red, green and blue.

    Default value: `00805C`

* `GraphShadowColor`

    Defines the color of the legend shadow for all graphs
    generated by Stone Steps Webalizer. The value must be
    specified as six hexadecimal digits, two for each color -
    red, green and blue.

    Default value: `333333`

* `GraphTitleColor`

    Defines the color of graph titles. The value must be
    specified as six hexadecimal digits, two for each color -
    red, green and blue.

    Default value: `0000FF`

* `GraphTrueColor`

    Specifies whether Stone Steps Webalizer will create
    TrueColor or palette-based graph images. TrueColor images
    are larger in size, but are of better quality, especially if
    font smoothing is turned on.

    Default value: `no`

* `JavaScriptCharts`

    Specifies the type the the JavaScript chart package to use.
    Only Highcharts is supported at the moment. By default this
    value is empty and all charts are rendered as PNG images.

    If you configure this value, you also need to configure the
    JavaScript directory via `HTMLJsPath`.

    Please note that Highcharts is not an Open Source package
    and may be used without buying a license only for non-commercial
    purposes. Read Highcharts` page for additional information
    about their licensing before enabling Highcharts in the reports:

    https://shop.highsoft.com/faq#Non-Commercial-0

    For more information about the JavaScript charts package used
    in this implementation see this page:

    https://www.highcharts.com/products/highstock

* `JavaScriptChartsPath`

    JavaScripts charts by default will use implementation scripts
    from the 3rd-party chart vendor's location. If you would like
    to serve charts scripts from your servers or use some alternative,
    JavaScript charts integration, specify as many of these variables
    as you need in the configuration file and each will be output in
    the head element of each report.

### Database Configuration Options

* `BatchProcessing`, `Batch`

    Instructs Stone Steps Webalizer to run in a batch mode,
    which avoids generating report files at the end of each
    run. Once the last log file has been processed, a
    monthly report may be generated with the command line
    argument `--prepare-report`.

    Default value: `no`  
    Command line argument: `--batch`

* `DbDirect`

    Configures Berkeley DB not to use the operating system
    caching and use only the internal Berkeley DB caching
    mechanism. This may help reduce memory footprint on some
    systems.

    Default value: `no`

* `DbSeqCacheSize`

    Is the number of cached DB sequence numbers used by
    Stone Steps Webalizer. The default value of 100 is
    sufficient in most cases.

* `DbCacheSize`

    This configuration value is used as a guiding number to
    limit the amount of memory used by Stone Steps Webalizer.
    The specified value is not used as a hard memory limit,
    but rather as a base value to initialize various components,
    such as Berkeley DB cache and internal memory tables for
    log file items, such as hosts and URLs. The actual amount
    of memory used may be about three-four times as much, but
    it should level at some point. You may need to experiment
    with different values to arrive at an acceptable limit.
    Values may be suffixed with K, M or G for kilo, mega and
    giga multipliers. The minimum value is `1 MB`.

    Default value: `50 MB`

* `DbPath`

    Specifies a fully-qualified directory path where the state
    database will be created.

    Default value: output directory

* `DbName`

    Sets the name of the database file.

    Default value: `webalizer`

* `DbExt`

    Sets the extension of the database file.

    Default value: `db`

### Top Table Keywords

* `TopAgents`

    This allows you to specify how many "Top" user agents are
    displayed in the "Top User Agents" table.  The default
    is 15.  If you do not want to display user agent statistics,
    specify a value of zero (`0`).  The display of user agents
    will only work if your web server includes this information
    in its log file (ie: a combined log format file).

    Command line argument: `-A`

* `AllAgents`

    Will cause a separate HTML page to be generated for all
    normally visable User Agents.  A link will be added to
    the bottom of the "Top User Agents" table if enabled.
    Value can be either `yes` or `no`, with `no` being the
    default.

* `TopCountries`

    This allows you to specify how many "Top" countries are
    displayed in the "Top Countries" table.  The default is
`30`.  If you want to disable the countries table, specify
    a value of zero (`0`).

    Command line argument: `-C`

* `TopCities`

    Limits the number of cities listed in the Top Cities report.
    This report is generated only if GeoIP is configured and if
    GeoIPCity is set to `yes`. Set TopCities to zero if you would
    like to disable the city report, but show city names in other
    reports.

    Default value: `30`

* `TopReferrers`

    This allows you to specify how many "Top" referrers are
    displayed in the "Top Referrers" table.  The default is
    30.  If you want to disable the referrers table, specify
    a value of zero (0).  The display of referrer information
    will only work if your web server includes this information
    in its log file (ie: a combined log format file).

    Command line argument: `-R`

* `AllReferrers`

    Will cause a separate HTML page to be generated for all
    normally visable Referrers.  A link will be added to the
"Top Referrers" table if enabled.  Value can be either
`yes` or `no`, with `no` being the default.

* `TopHosts`

    This allows you to specify how many "Top" hosts are
    displayed in the "Top Hosts" table.  The default is 30.
    If you want to disable the hosts table, specify a value
    of zero (0).

    Command line argument: `-S`

* `TopKHosts`

    Identical to TopSites, except for the `by KByte` table.
    Default is 10.  No command line switch for this one.

* `AllHosts`

    Will cause a separate HTML page to be generated for all
    normally visible Sites.  A link will be added to the
    bottom of the "Top Sites" table if enabled.  Value can
    be either `yes` or `no`, with `no` being the default.

* `TopURLs`

    This allows you to specify how many "Top" URL's are
    displayed in the "Top URL's" table.  The default is `30`.
    If you want to disable the URL's table, specify a value
    of zero (`0`).

    Command line argument: `-U`

* `TopKURLs`

    Identical to TopURLs, except for the `by KByte` table.
    Default is `10`.  No command line switch for this one.

* `AllURLs`

    Will cause a separate HTML page to be generated for all
    normally visible URLs.  A link will be added to the
    bottom of the "Top URLs" table if enabled.  Value can
    be either `yes` or `no`, with `no` being the default.

* `TopEntry`

    Allows you to specify how many "Top Entry Pages" are
    displayed in the table.  The default is `10`.  If you
    want to disable the table, specify a value of zero (0).

    Command line argument: `-e`

* `TopExit`

    Allows you to specify how many "Top Exit Pages" are
    displayed in the table.  The default is `10`.  If you
    want to disable the table, specify a value of zero (`0`).

    Command line argument: `-E`

* `TopSearch`

    Allows you to specify how many "Top Search Strings" are
    displayed in the table.  The default is `20`.  If you
    want to disable the table, specify a value of zero (`0`).
    Only works if using combined log format (ie: contains
    referrer information).

* `TopUsers`

    This allows you to specify how many "Top" usernames are
    displayed in the "Top Usernames" table.  Usernames are
    only available if you use http authentication on your
    web server. The default value is `20`. If you want to
    disable the Username table, specify a value of zero (`0`).

* `AllUsers`

    Will cause a separate HTML page to be generated for all
    normally visible user names.  A link will be added to the
    bottom of the "Top Usernames" table if enabled.  Value
    can be either `yes` or `no`, with `no` being the default.

* `AllSearchStr`

    Will create a separate HTML page to be generated for all
    normally visable Search Strings.  A link will be added
    to the bottom of the "Top Search Strings" table if
    enabled.  Value can be either `yes` or `no`, with `no`
    being the default.

* `AllDownloads`

    If this configuration parameter is set to yes and the number
    of downloads is greater than the number of lines in the
    download report (i.e. greater than the value of
    TopDownloads), Stone Steps Webalizer will generate a
    standalone downloads report, listing all downloads
    for the current month. 

    Default value: `no`

* `AllErrors`

    If this configuration parameter is set to yes and the number
    of HTTP errors is greater than the number of lines in the
    HTTP error report (i.e. greater than the value of
    TopErrors), Stone Steps Webalizer will generate a standalone
    HTTP error report, listing all HTTP errors for the current
    month. 

    Default value: `no`

* `TopDownloads`

    Defines the maximum number of lines in the downloads report.
    If the number of actual downloads is greater than this
    value, the rest of the downloads will either be discarded or
    generated as a separate downloads report, depending on the
    value of AllDownloads. 

    Default value: `20`

* `TopErrors`

    Defines the maximum number of lines in the HTTP error
    report. If the number of actual errors is greater than this
    value, the rest of the errors will either be discarded or
    generated as a separate HTTP error report, depending on the
    value of AllErrors. 

    Default value: `20`

* `TopASN`

    Defines the maximum number of rows in the top Autonomous
    System report. 

    Default value: `30`


### Hide Object Keywords

These keywords allow you to hide user agents, referrers, hosts, URL's
and usernames from the various "Top" tables.  The value for these keywords
are the same as those used in their command line counterparts.  You
can specify as many of these as you want without limit.  Refer to the
section above on "Command Line Options" for a description of the string
formatting used as the value.  Values cannot exceed 80 characters in
length.

* `HideAgent`

    This allows specified user agents to be hidden from the
    "Top User Agents" table.  Not very useful, since there
    a zillion different names by which browsers go by today,
    but could be useful if there is a particular user agent
    (ie: robots, spiders, real-audio, etc..) that hits your
    site frequently enough to make it into the top user agent
    listing.  This keyword is useless if 1) your log file does
    not provide user agent information or 2) you disable the
    user agent table.

    Command line argument: `-a`

* `HideReferrer`

    This allows you to hide specified referrers from the
    "Top Referrers" table.  Normally, you would only specify
    your own web server to be hidden, as it is usually the
    top generator of references to your own pages.  Of course,
    this keyword is useless if 1) your log file does not include
    referrer information or 2) you disable the top referrers
    table.

    Command line argument: `-r`

* `HideHost`

    This allows you to hide specified hosts from the "Top
    Hosts" table.  Normally, you would only specify your own
    web server or other local machines to be hidden, as they
    are usually the highest hitters of your web site, especially
    if you have their browsers home page pointing to it.

    Command line argument: `-s`

* `HideAllHosts`

    This allows hiding all individual hosts from the display,
    which can be useful when a lot of groupings are being
    used (since grouped records cannot be hidden).  It is
    particularly useful in conjunction with the GroupDomains
    feature, however can be useful in other situations as well.
    Value can be either `yes` or `no`, with `no` the default.

    Command line argument: `-X`

* `HideURL`

    This allows you to hide URL's from the "Top URL's" table.
    Normally, this is used to hide items such as graphic files,
    audio files or other non-HTML files that are transferred
    to the visiting user.

    Command line argument: `-u`

* `HideUser`

    This allows you to hide Usernames from the "Top Usernames"
    table.  Usernames are only available if you use http based
    authentication on your web server.

* `HideRobots`

    If set to `yes`, this option allows you to hide all robots
    from the Top Hosts and Top Agents reports. Robot groups, if
    there are any, will still be displayed in the Top Agents
    report. Use the Robot configuration parameter to identify
    robots.

    Default value: `no`

* `HideGroupedItems`

    Controls whether items that are being currently grouped with
    one of the grouping configuration variables are also added to
    the corresponding hide item list or not.

    This configuration value may be used multiple times between
    multiple sets of group configuration values. For example,
    in this configuration `FireFox`, `Chrome` and `Opera` user
    agents will be reported in a group and individually, but
    Internet Explorer will be reported only as a group.

        GroupAgent         Chrome
        GroupAgent         Firefox

        HideGroupedItems   yes

        GroupAgent         MSIE        Internet Explorer

        HideGroupedItems   no

        GroupAgent         Opera

    Default value: `no`

### Group Object Keywords

The Group* keywords allow object grouping based on Host, URL,
Referrer, User Agent and Usernames.  Combined with the Hide* keywords,
you can customize exactly what will be displayed in the `Top` tables.

For example, to only display totals for a particular directory, use a
GroupURL and HideURL with the same value (ie: `/help/*`).  Group
processing is only done after the individual record has been fully
processed, so name mangling and site total updates have already been
performed. Because of this, groups are not counted in the main site
total (as that would cause duplication). Groups can be displayed in
bold and shaded as well. Grouped records are not, by default, hidden
from the report.  This allows you to display a grouped total, while
still being able to see the individual records, even if they are part
of the group.  If you want to hide the detail records, follow the
Group* directive with a Hide* one using the same value. There
are no command line switches for these keywords.  The Group* keywords
also accept an optional label to be displayed instead of the actual
value used. This label should be separated from the value by at least
one whitespace character, such as a space or tab character.  See the
sample.conf file for examples.

GroupReferrer Allows grouping Referrers.  Can be handy for some of the
major search engines that have multiple host names a
referral could come from.

* `GroupURL`

    This keyword allows grouping URL's. Useful for grouping
    complete directory trees.

* `GroupHost`

    This keywords allows grouping Sites.  Most used for
    grouping top level domains and unresolved IP address
    for local dial-ups, etc...

* `GroupAgent`

    Groups User Agents.  You could use `Firefox`, `Chrome`
    and `Edge` as the values for `GroupAgent` and `HideAgent`
    keywords.  Make sure you put `Edge` first because it is
    based on Chrome and also lists `Chrome` in its user agent
    string.

* `GroupDomains`

    Allows automatic grouping of domains.  The numeric value
    represents the level of grouping, and can be thought of
    as 'the number of dots' to display.  A `1` will display
    second level domains only (`xxx.xxx`), a `2` will display
    third level domains (`xxx.xxx.xxx`) etc...  The default
    value of `0` disables any domain grouping.

    Command line argument: `-g`

* `GroupUser`

    Allows grouping of usernames.  Combined with a group
    name, this can be handy for displaying statistics on
    a particular group of users without displaying their
    real usernames.

* `GroupShading`

    Allows shading of table rows for groups.  Value can be
    `yes` or `no`, with the default being `yes`.

    GroupHighlight Allows bolding of table rows for groups.  Value can be
    `yes` or `no`, with the default being `yes`.

* `GroupRobots`

    If set to `yes`, will instruct Stone Steps Webalizer to
    group automated user agents (robots) in the Top Agents
    report. Each group will be assigned a CSS class `robot`
    to distinguish them from non-robot user agents.  Use the
    Robot configuration parameter to identify robots.

    Default value: `no`


### Ignore/Include Object Keywords

These keywords allow you to completely ignore log records when generating
statistics, or to force their inclusion regardless of ignore criteria.
Records can be ignored or included based on host, URL, user agent, referrer
and username.  Be aware that by choosing to ignore records, the accuracy of
the generated statistics become skewed, making it impossible to produce
an accurate representation of load on the web server.  These keywords
behave identical to the Hide* keywords above, where the value can have
a leading or trailing wildcard `*`.  These keywords, like the Hide* ones,
have an absolute limit of `80` characters for their values.  These keywords
do not have any command line switch counterparts, so they may only be
specified in a configuration file.  It should also be pointed out that
using the Ignore/Include combination to selectively exclude an entire
site while including a particular `chunk` is _extremely_ inefficient,
and should be avoided.  Try grep'ing the records into a separate file
and process it instead.

* `IgnoreHost`

    This allows specified hosts to be completely ignored
    from the generated statistics.

* `IgnoreURL`

    This allows specified URL's to be completely ignored from
    the generated statistics.  One use for this keyword would
    be to ignore all hits to a `temporary` directory where
    development work is being done, but is not accessible to
    the outside world.

    Unlike other ignore keywords, IgnoreURL can take optional
    search argument names and values. Multiple IgnoreURL entries
    can be used to specify different search arguments for the
    same URL. If multiple IgnoreURL values are used, they must
    follow one another in the configuration file or those that
    are out of order will be ignored.

    The entire search argument name is matched, not a part of it,
    so `abc` will only match `abc` and not `abcd`.

    For example, if you would like to ignore `index.html` with search
    arguments `abc` and `xyz`, you would use these configuration variables:

        IgnoreURL    /index.html*   abc
        IgnoreURL    /index.html*   xyz

    This will ignore log records containing these URLs:

        /index.html?abc=123&x=1&y=2
        /index.html?x=1&y=2&xyz=123

    , but will process and report these URLs:

        /index.html?abcd=123&x=1&y=2
        /index.html?x=1&y=2

    `IgnoreURL` may also include search argument values, in which case
    both, name and value must match for a log line to be ignored.

    For example, if the following `IgnoreURL` entry is used in the
    configuration file:

        IgnoreURL    /index.html*   page=/test

    , then a log line containing this URL will be egnored:

        index.html?abc=123&page=/test&xyz=456

    , but a log record containing this URL will be processed and
    will appear in the report:

        index.html?abc=123&page=/catalog&xyz=456

    Note that search argument filtering is done before the ignore
    logic is applied, so if you filtered out the argument that is
    used in one of the IgnoreURL entries, log records containing
    excluded search arguments will not be ignored.

    See URL Normalization for additional details on what URL characters
    should be used in ignore patterns.

    IgnoreReferrer This allows records to be ignored based on the referrer
    field.

* `IgnoreAgent`

    This allows specified User Agent records to be completely
    ignored from the statistics.  Maybe useful if you really
    don't want to see all those hits from MSIE :)

* `IgnoreUser`

    This allows specified username records to be completely
    ignored from the statistics.  Usernames can only be used
    if you use http authentication on your server.

* `IncludeHost`

    Force the record to be processed based on hostname.
    This takes precedence over the `Ignore*` keywords.

* `IncludeURL`

    Force the record to be processed based on URL.  This takes
    precedence over the `Ignore*` keywords.

    IncludeReferrer Force the record to be processed based on referrer.
    This takes precedence over the `Ignore*` keywords.

* `IncludeAgent`

    Force the record to be processed based on user agent.
    This takes precedence over the `Ignore*` keywords.

* `IncludeUser`

    Force the record to be processed based on username.
    Usernames are only available if you use http based
    authentication on your server.  This takes precedence over
    the `Ignore*` keywords.

* `IgnoreRobots`

    If set to `yes`, forces all records submitted by a robot
    user agent to be completely ignored. Use the `Robot`
    configuration parameter to identify robots.

    Default value: `no`


### Dump Object Keywords

The Dump* Keywords allow text files to be generated that can then be used
for import into most database, spreadsheet and other external programs.
The file is a standard tab delimited text file, meaning that each column
is separated by a tab (0x09) character.  A header record may be included
if required, using the `DumpHeader` keyword.  Since these files contain
all records that have been processed, including normally hidden records,
an alternate location for the files can be specified using the `DumpPath`
keyword, otherwise they will be located in the default output directory.

* `DumpPath`

    Specifies an alternate location for the dump files.  The
    default output location will be used otherwise.  The value
    is the path portion to use, and normally should be an
    absolute path (ie: has a leading `/` character), however
    relative path names can be used as well, and will be
    relative to the output directory location.

* `DumpExtension`

    Allows the dump filename extensions to be specified. The
    default extension is `tab`, however may be changed with
    this option.

* `DumpHeader`

    Allows a header record to be written as the first record
    of the file.  Value can be either `yes` or `no`,  with
    the default being `no`.

* `DumpHosts`

    Dump tab-delimited hosts file.  Value can be either
    `yes` or `no`, with the default being `no`. The filename
    used is `site_YYYYMM.tab` (YYYY=year, MM=month).

* `DumpURLs`

    Dump tab delimited url file.  Value can be either `yes` or
    `no`, with the default being `no`.  The filename used is
    `url_YYYYMM.tab` (YYYY=year, MM=month).

* `DumpReferrers`

    Dump tab delimited referrer file.  Value can be either
    `yes` or `no`, with the default being `no`.  Filename
    used is `ref_YYYYMM.tab` (YYYY=year, MM=month).  Referrer
    information is only available if present in the log
    file (ie: combined web server log).

* `DumpAgents`

    Dump tab delmited user agent file.  Value can be either
    `yes` or `no`, with the default being `no`.  Filename
    used is `agent_YYYYMM.tab` (YYYY=year, MM=month).  User
    agent information is only available if present in the
    log file (ie: combined web server log).

* `DumpUsers`

    Dump tab delimited username file.  Value can be either
    `yes` or `no`, with the default being `no`.  FIlename
    used is `user_YYYYMM.tab` (YYYY=year, MM=month).  The
    username data is only avilable if http authentication
    is used on the web server and that information is present
    in the log.

* `DumpSearchStr`

    Dump tab delimited search string file.  Value can be
    either `yes` or `no`, with the default being `no`.
    Filename used is `search_YYYYMM.tab` (YYYY=year, MM=month).
    the search string data is only available if referrer
    information is present in the log being processed and
    recognized search engines were found and processed.

* `DumpDownloads`

    If this configuration parameter is set to yes, Stone Steps
    Webalizer will generate a tab-delimited file listing all
    downloads for the current month. 

    Default value: `no`

* `DumpErrors`

    If this configuration parameter is set to yes, Stone Steps
    Webalizer will generate a tab-delimited file listing all
    HTTP errors for the current month. 

    Default value: `no`

* `DumpCountries`

    Generate a tab-delimited data file for all countries. The
    file name will be `country_YYYYMM.tab`.

    Default value: `no`

* `DumpCities`

    Generate a tab-delimited data file for all cities. The file
    name will be `city_YYYYMM.tab`.

    Default value: `no`

* `DumpASN`

    Generates a tab-delimited data file for all ASN entries.
    The file will be named `asn_YYYYMM.tab`.

    Default value: `no`

### HTML Generation Keywords

These keywords allow you to customize the HTML code that The Webalizer
produces, such as adding a corporate logo or links to other web pages.
You can specify as many of these keywords as you like, and they will be
used in the order that they are found in the file.  Values cannot exceed
80 characters in length, so you may have to break long lines up into two
or more lines.  There are no command line counterparts to these keywords.

* `HTMLExtension`

    Allows generated pages to use something other than the
    default `html` extension for the filenames.  Do not
    include the leading period (`.`) when you specify the
    extension.

    Command line argument: `-x`

* `HTMLPre`

    Allows code to be inserted at the very beginning of the
    HTML files. Be careful not to include any HTML here, as it
    is inserted _before_ the `<html>` tag in the file.  Use it
    for server-side scripting capabilities, such as php3, to
    insert scripting files and other directives.

    It is an error to have the `<!doctype>` tag in any HTMLPre
    entries.

* `HTMLHead`

    Allows you to insert HTML code between the `<head></head>`
    block.  There is no default.  Useful for adding scripts
    to the HTML page, such as Javascript or php3, or even
    just for adding a few META tags to the document.

* `HTMLBody`

    This keyword defines HTML code to be placed immediately
    after the start <body> tag of the report, just before the
    title and "summary period/generated on" lines. Keep in
    mind the placement of this code in relation to the title
    and other aspects of the web page.  A typical use is to
    add a corporate logo (graphic) in the top right.

    It is an error to have the `<body>` tag in any `HTMLBody`
    entries.

* `HTMLPost`

    This keyword defines HTML code that is placed after the
    title and "summary period/generated on" lines, just before
    the initial horizontal rule `<hr>` tag.  Normally this keyword
    isn't needed, but is provided in case you included a large
    graphic or some other weird formatting tag in the HTMLHead
    section that needs to be cleaned up or terminated before the
    main report section.

* `HTMLTail`

    This keyword defines HTML code that is placed at the bottom
    of the report. Normally this keyword is used to provide a link
    back to your home page or insert a small graphic at the bottom
    right of the page.

* `HTMLEnd`

    This allows insertion of closing code for `HTMLBody` tags, at
    the very end of the page.

    It is an error to have the closing `</body>` tag in any
    `HTMLEnd` entries.

* `HTMLCssPath`

    Specifies a URL path to the webalizer.css file, not
    including the file name. The path must be a URL path,
    even if it refers to a local file. You can reference
    one CSS file in many reports to make it easier to
    change report layout in one place.

    Default value: none

* `HTMLJsPath`

    Specifies a URL path to the `webalizer.js` file, not
    including the file name. The path must be a URL path,
    even if it refers to a local file. You can reference
    one JavaScript file in many reports to make it easier
    to change report layout in one place.

    Default value: none

* `HTMLMetaNoIndex`

    Controls whether Stone Steps Webalizer will generate HTML
    reports that may be indexed by robots or not.

    Default value: `yes`

* `HTMLExtensionLang`

    Configures Stone Steps Webalizer to append the current
    language code to the generated HTML and image files, so
    Apache language extensions can be used to browse language-
    specific reports. 

    For example, if the current language is Japanese, `index.html`
    will be named `index.html.ja`.

    Default value: `no`

## Notes on Web Log Files

Stone Steps Webalizer supports W3C, IIS, Apache, CLF and Squid log formats.

Avoid processing the same log files more than once because in every subsequent
run Stone Steps Webalizer will use the latest processed log time stamp to skip
log lines that have already been processed in previous runs and, considering
that modern log files will most likely contain multiple log lines with the same
time stamp value, some of those log lines had been processed before and some have
not yet been processed, but will be discarded because of the matching time stamp
values anyway.

When configuring your web server to rotate log files, keep in mind that as soon
as a log line with the time stamp from the next month is processed, the current
month will be ended and reports will be generated. If you would like to end the
current month after processing the log file you know to be the last one, use
`--end-month` switch to do so and then `--prepare-report` to generate the monthly
report from the rolled over state database. If you choose to use this workflow
and intend to process multiple log files one after another, you will achieve better
performance using the `--batch` switch, which prevents Stone Steps Webalizer from
generating intermediate monthly reports after each log file.

### IIS and W3C

W3C Extended Log File Format (W3C) defines special directives describing
the physical structure of the log file. Stone Steps Webalizer recognizes
`#Fields` directives and dynamically reconfigures its parser to process log
file entries following this directive in the matching order.

IIS log format mostly follows the W3C standard, with one excepion - it
outputs request processing time (`time-taken`) in milliseconds instead of
seconds.

### Apache 

Apache logs may be customized using LogFormat and CustomLog directives
(these are Apache configuration keywords, not those used by Stone
Steps Webalizer). Stone Steps Webalizer can parse the CustomLog
directive, if it's specified anywhere in the configuration using the
ApacheLogFormat configuration parameter.

For example (the line is broken for display purposes; it would actually
appear as a single line in the configuration file):

    ApacheLogFormat
          %a %l \"%u\" %t %m "%U" \"%q\" %p %>s %b %D
          \"%{Referer}i\" \"%{User-Agent}i\"

In the preceding example the user name field (`%u`) is enclosed in
quotes because user names may contain spaces. The URL stem field (`%U`)
is quoted as well because Apache logs URL file paths decoded and URLs
may contain spaces. The query string field (`%q`) is quoted because
it may be reported as an empty string. Numeric fields, on the other
hand, such as request processing time (`%D`), do not need to be quoted.

It is important to understand that Apache log files do not contain log
format information (unlike log files in W3C extended format) and
switching log file format without renaming the current log file will
result in a log file that contains log information in mixed formats.
Such log files cannot be analyzed unless they are split onto multiple
consistently-formatted log files.

If log formats specified in `httpd.conf` and `ssl.conf` for any shared log
file are not the same, the resulting log file will contain log
information in mixed formats and cannot be analyzed. We also recommend
that you use the `%p` field (port number), as shown in the example
above, to make it possible to distinguish HTTP and HTTPS requests.

### Common Log Format (CLF)

The Webalizer supports CLF log formats, which should work for just
about everyone.  If you want User Agent or Referrer information, you
need to make sure your web server supplies this information in it's
log file, and in a format that the Webalizer can understand.  While
The Webalizer will try to handle many of the subtle variations in
log formats, some will not work at all.   Most web servers output
CLF format logs by default.

For Apache, in order to produce the proper log format, add the following
to the  httpd.conf   file:

    LogFormat "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-agent}i\""

This instructs the Apache web server to produce a `combined` log
that includes the referrer and user agent information on the end of
each record, enclosed in quotes (This is the standard recommended
by both Apache and NCSA).

## Referrers

Referrers are weird critters... They take many shapes and forms, which makes
it much harder to analyze than a typical URL, which at least has some
standardization.  What is contained in the referrer field of your log
files varies depending on many factors, such as what site did the referral,
what type of system it comes from and how the actual referral was generated.
Why is this?  Well, because a user can get to your site in many ways... They
may have your site bookmarked in their browser, they may simply type your
sites URL field in their browser, they could have clicked on a link on some
remote web page or they may have found your site from one of the many search
engines and site indexes found on the web.  The Webalizer attempts to deal
with all this variation in an intelligent way by doing certain things to
the referrer string which makes it easier to analyze.  Of course, if your
web server doesn't provide referrer information, you probably don't really
care and are asking yourself why you are reading this section...

Most referrer's will take the form of `http://somesite.com/somepage.html`,
which is what you will get if the user clicks on a link somewhere on the
web in order to get to your site.  Some will be a variation of this, and
look something like `file:/some/such/sillyname`, which is a reference from
a HTML document on the users local machine.  Several variations of this can
be used, depending on what type of system the user has, if he/she is on
a local network, the type of network, etc...  To complicate things even
more, dynamic HTML documents and HTML documents that are generated by
CGI scripts or external programs produce lots of extra information which
is tacked on to the end of the referrer string in an almost infinite number
of ways.  If the user just typed your URL into their browser or clicked on
a bookmark, there won't be any information in the referrer field and will
take the form `-`.

In order to handle all these variations, The Webalizer parses the referrer
field in a certain way.  First, if the referrer string begins with `http`,
it assumes it is a normal referral and converts the `http://` and following
hostname to lowercase in order to simplify hiding if desired.

For example, the referrer

    HTTP://WWW.MyHost.Com/This/Is/A/HTML/Document.html

will become

    http://www.myhost.com/This/Is/A/HTML/Document.html

Notice that only the `http://` and hostname are converted to lower case.
The rest of the referrer field is left alone. This follows standard
convention, as the actual method (HTTP) and hostname are always case
insensitive, while the document name portion is case sensitive.

Referrers that came from search engines, dynamic HTML documents, CGI
scripts and other external programs usually tack on additional information
that it used to create the page.  A common example of this can be found
in referrals that come from search engines and site indexes common on the
web.  Sometimes, these referrers URL's can be several hundred characters
long and include all the information that the user typed in to search for
your site.  The Webalizer deals with this type of referrer by stripping
off all the query information, which starts with a question mark `?`.
The Referrer `http://search.yahoo.com/search?p=usa%26global%26link` will
be converted to just `http://search.yahoo.com/search`.

When a user comes to your site by using one of their bookmarks or by
typing in your URL directly into their browser, the referrer field is
blank, and looks like `-`.  Most sites will get more of these referrals
than any other type.  The Webalizer converts this type of referral into
the string `- (Direct Request)`.  This is done in order to make it easier
to hide via a command line option or configuration file option.  This is
because the character `-` is a valid character elsewhere in a referrer
field, and if not turned into something unique, could not be hidden without
possibly hiding other referrers that shouldn't be.

Stone Steps Webalizer supports a configuration parameter,
`SpamReferrer`, which lists referrer patterns considered as spam.
Visitors submitting these requests will be red-flagged and marked in
the hosts report as spammers.

Multiple `SpamReferrer` entries may be used to specify more than one
pattern. For example, the first two entries below will red-flag all
requests with the referrer URL containing words gambling or poker
anywhere in the referrer URL. The third entry will match only if the
referrer URL begins with the string of characters preceding the
asterisk.

    SpamReferrer    gambling
    SpamReferrer    casino
    SpamReferrer    http://www.instantlinkexchange.com*

Once a visitor is identified as a spammer, all requests from this
IP address will be treated as spam for the rest of the currently-
reported month. Spam requests will be counted as usual in all reports,
except the referrer report, to prevent spam referrers from appearing
in the report as clickable links. Spamming hosts will also highlighted
in red color in the hosts report.

If you would like to change the color of the highlighting, locate the
following line in webalizer.css and change the color to any other
value:

    td.spammer, span.spammer {color: red;}

In addition to highlighting, the all-hosts and the tab-separated host
reports will have an asterisk output next to the spammer's host.

## URL Normalization

In general, URLs are supposed to be uniformly encoded in such a way that keeps them
simple, but still usable even if they are printed on paper, pronounced on the radio,
or appear in other contexts where it may be impossible to distinguish characters
from different languages. This encoding is described by the internet standard
RFC-3986 and defines which characters may appear in their natural representation
and which should be percent-encoded as one or more sequences of a percent character
followed by two hexadecimal digits that represent that character (e.g. `&` is encoded
as `%26`).

Sometimes URL characters may be encoded incorrectly, which may be because of various
historical reasons, or because of bugs in user agents, or in an attempt to avoid simple
spam filters that do not percent-decode URL sequences before looking for spam keywords.
In either case, having the same URL encoded differently fragments reports by creating
aliases (e.g. `/xAy/` and `/x%41y/` are counted as two URLs) and makes spam detection more
difficult because all possible aliases must be filtered individually. In order to deal
with these issues, Stone Steps Webalizer normalizes all URLs extracted from log files
to reduce aliasing and improve report readability.

IMPORTANT: A normalized URL is not a well-formed URL in the sense that it should not
be used verbatim in HTML as a copy-and-paste href link, but it could be used in the
URL field in a web browser because URL normalization does not change the meaning of
existing URL components, but merely makes them more readable.

For example, a normalized URL `/?q="abc"` is more readable than the equivalent
well-formed URL `/?q=%22abc%22`, but if it is copy-pasted into an href attribute
in HTML without proper HTML encoding, it will break that HTML because the double
quotes in the URL will interfere with double quotes in the HTML attribute.

URL normalization is performed before any other work is done against all URLs, which
follows the rules described below, so all configuration filters should use normalized
characters in all ignore, hide and group URL patterns.

  * Following characters are not encoded and if they are found in the encoded form,
    such as `%41` instead of the character `A`, they will be decoded to their original
    form shown below. If any of these characters is percent-encoded, it will be
    decoded.

      `A-Z`, `a-z`, `0-9`, `-` `.` `_` `~` `"` `%` `<` `>` space

  * Following characters have special meaning within URLs and will not be encoded
    or decoded and will remain in their current form, whatever it is (i.e. `a&b` and
    `a%26b` will remain exactly as they were before normalization).

     `:` `/` `?` `#` `[` `]` `@` `!` `$` `&` `` ` `` `(` `)` `*` `+` `,` `;` `=` `%`

  * Percent-encoded control characters will not be decoded and unencoded control
    characters will be percent-encoded.

  * Percent-encoded multibyte UTF-8 characters will be decoded to their UTF-8 form.

  * Percent-encoded non-UTF-8 characters are not allowed in URLs and will be decoded
    as if they were characters from the Latin-1 (Western) alphabet and converted to
    UTF-8.

  * A percent character that is not a part of a percent-encoding sequence in a URL
    will be percent-encoded (e.g. `/a%b/` will become `/a%25b/`).

If you intend to filter URLs with spaces, make sure to use `EnablePhraseValues`, so the
space in the pattern wouldn't be misinterpreted as a pattern/value separator. In other
words, if `EnablePhraseValues` is not enabled, the following pattern with a space will
be interpreted as as a URL `"*/ab"` with a search argument `"cd/"`, as in `"/ab/?cd/"`.

    IgnoreURL  */ab cd/

Note that once some URL path pattern is found, only search arguments of the matching
path pattern will be checked, but no further. This may produce unexpected results if
broader URL path patterns (e.g. `*`) are placed first in the list of `IgnoreURL` filters.
Consider these filters:

    IgnoreURL   /abc/*
    IgnoreURL   /xyz/*   pageid=1
    IgnoreURL   /xyz/*   pageid=2
    IgnoreURL   /xyz/*   pageid=3
    IgnoreURL   /def/*

Let's say that the current URL is `/xyz/?x=1&pageid=2&y=2`. First, the URL path, which
is `/xyz/`, is checked against `/abc/*`, which will not match. Then `/xyz/` is checked
against the first occurrence of `/xyz/*`, which will match and will trigger a special
search mode that will stop searching URL path patterns after either all `/xyz/*`
patterns are checked or a matching search argument is found. In this example, it will
be the filter with `pageid=2`. Because the URL path matched, the next URL path pattern,
`def/*`, will not even be checked. This is done for performance reasons, so a long list
of ignore filters wouldn't slow down log processing too much.

Now, consider a slightly different list of filters:

    IgnoreURL   /abc/*
    IgnoreURL   /*   pageid=1
    IgnoreURL   /*   pageid=2
    IgnoreURL   /*   pageid=3
    IgnoreURL   /def/*

The pattern `/*` will match any URL, so if a URL in the log line is `/def/p.html`, the
pattern `/*` will match this URL path, but the actual URL doesn't have any search
arguments, so none of the three filters with pageid= will match. However, because `/*`
matches any URL, the `/def/*` pattern will not ever be checked.

One way to work this around is to have those broader filters at the very end, so all
other patterns are matched first, but this would be an error-prone approach if more
than one catch-all patterns is needed.

## Search String Analysis

  The Webalizer will do a minimal analysis on referrer strings that
it finds, looking for well known search string patterns.  Most of
the major search engines are supported, such as Yahoo!, Altavista,
Lycos, etc...  Unfortunately, search engines are always changing
their internal/CGI query formats, new search engines are coming on
line every day, and the ability to detect _all_ search strings is
nearly impossible.  However, it should be accurate enough to give
a good indication of what users were searching for when they stumbled
across your site.  Note: as of version 1.31, search engines can now
be specified within a configuration file.  See the sample.conf file
for examples of how to specify additional search engines.

## Visits/Entry/Exit Figures

The majority of data analyzed and reported on by The Webalizer is
as accurate and correct as possible based on the input log file.
However, due to the limitation of the HTTP protocol, the use of
firewalls, proxy servers, multi-user systems, the rotation of your
log files, and a myriad of other conditions, some of these numbers
cannot, without absolute accuracy, be calculated.  In particular,
Visits, Entry Pages and Exit Pages are suspect to random errors
due to the above and other conditions.  The reason for this is
twofold, 

  1) Log files are finite in size and time interval, and
  2) There is no way to distinguish multiple individual users apart
     given only an IP address.

Because log files are finite, they have a beginning and ending, which
can be represented as a fixed time period. There is no way of knowing
what happened previous to this time period, nor is it possible to
predict future events based on it.  Also, because it is impossible
to distinguish individual users apart, multiple users that have the
same IP address all appear to be a single user, and are treated as
such. This is most common where corporate users sit behind a
proxy/firewall to the outside world, and all requests appear to come
from the same location (the address of the proxy/firewall itself).
 Dynamic IP assignment (used with dial-up internet accounts) also
present a problem, since the same user will appear as to come from
multiple places.

For example, suppose two users visit your server from XYZ company,
which has their network connected to the Internet by a proxy server
`fw.xyz.com`.  All requests from the network look as though they
originated from `fw.xyz.com`, even though they were really initiated
from two separate users on different PC`s.  The Webalizer would
see these requests as from the same location, and would record only
1 visit, when in reality, there were two.  Because entry and exit
pages are calculated in conjunction with visits, this situation
would also only record 1 entry and 1 exit page, when in reality,
there should be 2.

As another example, say a single user at XYZ company is surfing
around your website..  They arrive at 11:52pm the last day of
the month, and continue surfing until 12:30am, which is now a
new day (in a new month).  Since a common practice is to rotate
(save then clear) the server logs at the end of the month, you
now have the users visit logged in two different files (current
and previous months).  Because of this (and the fact that the
Webalizer clears history between months), the first page the
user requests after midnight will be counted as an entry page.
This is unavoidable, since it is the first request seen by that
particular IP address in the new month.

For the most part, the numbers shown for visits, entry and exit
pages are pretty good "guesses", even though they may not be 100%
accurate.  They do provide a good indication of overall trends,
and shouldn't be that far off from the real numbers to count much.
You should probably consider them as the `minimum` amount possible,
since the actual (real) values should always be equal or greater
in all cases.


## Exporting Webalizer Data

The Webalizer now has the ability to dump all object tables to tab
delimited ASCII text files, which can then be imported into most
popular database and spreadsheet programs. The files are not normally
produced, as on some sites they could become quite large, and are only
enabled by the use of the Dump* configuration keywords.  The filename
extensions default to `.tab` however may be changed using the
`DumpExtension` keyword.  Since this data contains all items, even
those normally hidden, it may not be desirable to have them located
in the output directory where they may be visible to normal web users..
For this reason, the `DumpPath` configuration keyword is available,
and allows the placement of these files somewhere outside the normal
web server document tree.  An optional `header` record may be written
to these files as well, and is useful when the data is to be imported
into a spreadsheet.. databases will not normally need the header.  If
enabled, the header is simply the column names as the first record of
the file, tab separated.


## Language Support

Stone Steps Webalizer supports dynamic languages loaded at run time.
If the language file is found, its content will be used to produce
reports and progress messages. A new configuration variable,
LanguageFile, can be used to specify the location of the file. For
example

    LanguageFile    c:\tools\webalizer\lang\webalizer_lang.german

A typical language file contains series of name/value pairs. The name
identifies a text variable used by Stone Steps Webalizer and the value
provides language-specific text. For example, the English version of
the error message reported if a log file cannot be opened is defined
as follows:

    msg_log_err = Error: Can't open log file

Some language file entries, such as the list of months shown below,
may contain multiple elements. In this case, individual elements must
be separated by commas:

    s_month = Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec

The whitespace between the end of each element and the comma is
preserved and may be used for padding purposes. The whitespace
following the comma is stripped off, unless the element is enclosed in
double quotes.

If an individual element of a comma-separated list contains a comma,
as shown in the example below, this element must be enclosed in
double quotes:

    fr,       France,
    fx,       "France, Metropolitan",
    ga,       Gabon,

The file webalizer_lang.english contains additional information
about the structure of a language file.

Language files must be saved in the UTF-8 character encoding.

All existing language files have been converted to UTF-8. If you would
like to convert some other character encoding to UTF-8, you can use the
iconv utility.

For example, the following command converts a Japanese language file
from euc-jp to utf-8:

    $ iconv -f euc-jp -t utf-8 -o webalizer_lang.utf-8.japanese webalizer_lang.japanese

Stone Steps Webalizer may be configured to generate usage graphs using
TrueType fonts and UTF-8 character sets. In order to configure Stone
Steps Webalizer to use TrueType fonts, add `GraphFontNormal` and
`GraphFontBold` directives to the webalizer.conf file. Each of these
configuration variables must be a fully-qualified path to the selected
TrueType font file(s).

For example, the following two lines configure Stone Steps Webalizer
to use Lucida Console for all graph legends and axis markers and
Tahoma Bold for all graph titles:

    GraphFontNormal     c:\winnt\fonts\lucon.ttf
    GraphFontBold       c:\winnt\fonts\tahomabd.ttf

If GraphFontNormal and GraphFontBold are not specified, or if the
associated font files cannot be found, Stone Steps Webalizer will use
the default raster fonts to generate text for the graphs. Note that
raster fonts may not have suitable character representation for
non-Latin characters.

You can control the appearance of the generated text using three
configuration variables shown below. The first two variables define
the size of the normal and bold fonts (in points). The third one
instructs Stone Steps Webalizer whether to smooth font edges or not.

    GraphFontSmall      8
    GraphFontMedium     9.5
    GraphFontSmoothing  y

If you would like to use non-Latin UTF-8 characters in your language
files, make sure that the TrueType font you selected contains the
characters you need.

For example, Lucida Console shipped with the English version of
Windows does not have Japanese characters and if used to generate
graphs will result in unusable graphs.

## Robots

Robots are identified before user agents are mangled. Some robot related
features, such as highlighting robots in the Top Agents report, may be
disabled if agent mangling is active.

Log records matching `IgnoreRobot` entries, are completely ignored and
none of the robot-related entries are updated in this case.

Hosts are marked as robots when user agent matches one of the `Robot`
entries and only when a host is seen for the first time (i.e. when a
database host entry is created). If a human and a robot share the same
IP address, this address will be marked as robot or non-robot depending
on which user agent was active when the first hit was logged by the
web server.

Active visits are marked as robot visits when user agents matches one
of the `Robot` entries, regardless whether the corresponding hosts are
marked as robots or not. Visit robot flag is used when user agents
are classified as robots or not and when website and country totals
are updated. Country totals do not include robot activity.

## Known Issues

 * Country Totals

    Stone Steps Webalizer computes country totals at
    when ending visits. Consequently, in the incremental mode active
    visit data is not included into country totals until the last log
    file for the month is processed. The net effect of this is that
    the pie chart of all intermediate reports will show the Others
    slice bigger than it really is, because visit totals used as a 100%
    when computing pie slices are those of started visits. All active
    visits are terminated at the end of the month, so that the final
    pie chart accurately depicts the percentage of other countries.

 * Memory Usage

    The Webalizer makes liberal use of memory for internal
    data structures during analysis.  Lack of real physical memory will
    noticeably degrade performance by doing lots of swapping between memory
    and disk.  One user who had a rather large log file noticed that The
    Webalizer took over 7 hours to run with only 16 Meg of memory.  Once
    memory was increased, the time was reduced to a few minutes.


 * Performance

    The `Hide*`, `Group*`, `Ignore*`, `Include*` and `IndexAlias`
    configuration options can cause a performance decrease if lots of
    them are used.  The reason for this is that every log record must
    be scanned for each item in each list.

    For example, if you are Hiding 20 objects, Grouping 20 more, and
    Ignoring 5,  each record is scanned, at most, 46 times (`20+20+5+`
    an `IndexAlias` scan).

    On really large log files, this can have a profound impact.  It
    is recommended that you use the least amount of these configuration
    options that you can, as it will greatly improve performance.

## Final Notes

A lot of time and effort went into making The Webalizer, and to ensure that
the results are as accurate as possible.  If you find any abnormalities or
inconsistent results, bugs, errors, omissions or anything else that doesn't
look right, please let me know so I can investigate the problem or correct
the error.  This goes for the minimal documentation as well.  Suggestions
for future versions are also welcome and appreciated.

Visit Stone Steps Webalizer project on GitHub if you would like to log a
bug:

https://github.com/StoneStepsInc/StoneStepsWebalizer
