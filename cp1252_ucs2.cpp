#include "pch.h"

//
// Windows-1252 to UCS2 translation table
//
// HTML5 refers the W3C's character encoding standard, which defines that documents 
// labeled as Latin1 (ISO-8859-1) must be parsed as Windows code page 1252. The main 
// difference between the two is that CP-1252 uses the space occupied by control 
// characters in Latin1 to hold some Windows characters, such as smart quotes, etc.
//
// C++ 2003 does not allow characters from the basic character set appear in a 
// universal character name. For example, \u0041 or L'\u0041' are both considered 
// invalid because they both correspond to the character A from the basic character 
// set. C++ 2011 clarifies that only characters outside quoted characters or quoted 
// strings should be considered invalid, but some compilers still generate syntax 
// errors for universale character names like L'\u0041'. Use hexadecimal character 
// literals instead.
//
// The Unicode replacement character (U+FFFD) is used for Windows-1252 code points 
// that are not defined in CP1252.
//
extern const wchar_t CP1252_UCS2[256] = {
L'\x0000', L'\x0001', L'\x0002', L'\x0003', L'\x0004', L'\x0005', L'\x0006', L'\x0007', L'\x0008', L'\x0009', L'\x000A', L'\x000B', L'\x000C', L'\x000D', L'\x000E', L'\x000F', 
L'\x0010', L'\x0011', L'\x0012', L'\x0013', L'\x0014', L'\x0015', L'\x0016', L'\x0017', L'\x0018', L'\x0019', L'\x001A', L'\x001B', L'\x001C', L'\x001D', L'\x001E', L'\x001F', 
L'\x0020', L'\x0021', L'\x0022', L'\x0023', L'\x0024', L'\x0025', L'\x0026', L'\x0027', L'\x0028', L'\x0029', L'\x002A', L'\x002B', L'\x002C', L'\x002D', L'\x002E', L'\x002F', 
L'\x0030', L'\x0031', L'\x0032', L'\x0033', L'\x0034', L'\x0035', L'\x0036', L'\x0037', L'\x0038', L'\x0039', L'\x003A', L'\x003B', L'\x003C', L'\x003D', L'\x003E', L'\x003F', 
L'\x0040', L'\x0041', L'\x0042', L'\x0043', L'\x0044', L'\x0045', L'\x0046', L'\x0047', L'\x0048', L'\x0049', L'\x004A', L'\x004B', L'\x004C', L'\x004D', L'\x004E', L'\x004F', 
L'\x0050', L'\x0051', L'\x0052', L'\x0053', L'\x0054', L'\x0055', L'\x0056', L'\x0057', L'\x0058', L'\x0059', L'\x005A', L'\x005B', L'\x005C', L'\x005D', L'\x005E', L'\x005F', 
L'\x0060', L'\x0061', L'\x0062', L'\x0063', L'\x0064', L'\x0065', L'\x0066', L'\x0067', L'\x0068', L'\x0069', L'\x006A', L'\x006B', L'\x006C', L'\x006D', L'\x006E', L'\x006F', 
L'\x0070', L'\x0071', L'\x0072', L'\x0073', L'\x0074', L'\x0075', L'\x0076', L'\x0077', L'\x0078', L'\x0079', L'\x007A', L'\x007B', L'\x007C', L'\x007D', L'\x007E', L'\x007F', 
L'\x20AC', L'\xFFFD', L'\x201A', L'\x0192', L'\x201E', L'\x2026', L'\x2020', L'\x2021', L'\x02C6', L'\x2030', L'\x0160', L'\x2039', L'\x0152', L'\xFFFD', L'\x017D', L'\xFFFD', 
L'\xFFFD', L'\x2018', L'\x2019', L'\x201C', L'\x201D', L'\x2022', L'\x2013', L'\x2014', L'\x02DC', L'\x2122', L'\x0161', L'\x203A', L'\x0153', L'\xFFFD', L'\x017E', L'\x0178', 
L'\x00A0', L'\x00A1', L'\x00A2', L'\x00A3', L'\x00A4', L'\x00A5', L'\x00A6', L'\x00A7', L'\x00A8', L'\x00A9', L'\x00AA', L'\x00AB', L'\x00AC', L'\x00AD', L'\x00AE', L'\x00AF', 
L'\x00B0', L'\x00B1', L'\x00B2', L'\x00B3', L'\x00B4', L'\x00B5', L'\x00B6', L'\x00B7', L'\x00B8', L'\x00B9', L'\x00BA', L'\x00BB', L'\x00BC', L'\x00BD', L'\x00BE', L'\x00BF', 
L'\x00C0', L'\x00C1', L'\x00C2', L'\x00C3', L'\x00C4', L'\x00C5', L'\x00C6', L'\x00C7', L'\x00C8', L'\x00C9', L'\x00CA', L'\x00CB', L'\x00CC', L'\x00CD', L'\x00CE', L'\x00CF', 
L'\x00D0', L'\x00D1', L'\x00D2', L'\x00D3', L'\x00D4', L'\x00D5', L'\x00D6', L'\x00D7', L'\x00D8', L'\x00D9', L'\x00DA', L'\x00DB', L'\x00DC', L'\x00DD', L'\x00DE', L'\x00DF', 
L'\x00E0', L'\x00E1', L'\x00E2', L'\x00E3', L'\x00E4', L'\x00E5', L'\x00E6', L'\x00E7', L'\x00E8', L'\x00E9', L'\x00EA', L'\x00EB', L'\x00EC', L'\x00ED', L'\x00EE', L'\x00EF', 
L'\x00F0', L'\x00F1', L'\x00F2', L'\x00F3', L'\x00F4', L'\x00F5', L'\x00F6', L'\x00F7', L'\x00F8', L'\x00F9', L'\x00FA', L'\x00FB', L'\x00FC', L'\x00FD', L'\x00FE', L'\x00FF'
};
