#include <utility>

#include "ap_int.h"
#include "L1Trigger/L1TMuonEndCapPhase2/interface/Data/ActivationLut.h"

using namespace emtf::phase2;
using namespace emtf::phase2::data;

ActivationLut::ActivationLut() {
    prompt_pt_lut_ = {{
        0, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8075, 7942, 7818, 7693, 7569,
        7453, 7337, 7220, 7108, 6996, 6889, 6785, 6681, 6582, 6482, 6383, 6287, 6192, 6101, 6010, 5923, 5835, 5748, 5665,
        5582, 5500, 5421, 5342, 5263, 5189, 5110, 5039, 4965, 4894, 4824, 4757, 4687, 4620, 4554, 4488, 4426, 4363, 4301,
        4239, 4181, 4123, 4061, 4007, 3949, 3891, 3837, 3783, 3729, 3675, 3625, 3576, 3522, 3472, 3422, 3376, 3327, 3281,
        3231, 3186, 3140, 3095, 3053, 3007, 2966, 2920, 2879, 2837, 2796, 2754, 2717, 2676, 2638, 2597, 2560, 2522, 2485,
        2448, 2410, 2377, 2340, 2303, 2269, 2236, 2199, 2166, 2134, 2102, 2070, 2043, 2011, 1981, 1954, 1924, 1897, 1871,
        1842, 1816, 1791, 1766, 1741, 1717, 1696, 1671, 1647, 1627, 1604, 1583, 1560, 1541, 1521, 1502, 1479, 1460, 1441,
        1422, 1404, 1388, 1370, 1352, 1334, 1319, 1301, 1283, 1269, 1254, 1237, 1223, 1209, 1192, 1178, 1164, 1150, 1136,
        1123, 1109, 1096, 1082, 1069, 1056, 1046, 1033, 1020, 1007, 997, 984, 974, 962, 952, 939, 930, 920, 908, 898, 889,
        879, 867, 858, 848, 839, 830, 821, 812, 803, 794, 785, 776, 769, 760, 751, 743, 736, 727, 719, 712, 704, 695, 689,
        681, 674, 666, 660, 652, 646, 639, 631, 625, 619, 611, 605, 599, 593, 586, 580, 574, 568, 562, 557, 551, 545, 539,
        534, 528, 523, 517, 511, 506, 500, 495, 489, 484, 480, 475, 470, 464, 461, 455, 450, 446, 441, 436, 432, 427, 422,
        419, 413, 410, 405, 402, 396, 393, 388, 385, 380, 376, 371, 368, 365, 360, 357, 353, 348, 345, 342, 337, 334, 331,
        328, 323, 320, 316, 313, 309, 305, 302, 299, 296, 293, 290, 287, 282, 279, 276, 273, 270, 267, 264, 261, 258, 255,
        252, 249, 246, 243, 240, 237, 236, 233, 230, 227, 224, 221, 218, 217, 214, 211, 208, 205, 202, 201, 198, 195, 193,
        191, 188, 185, 183, 181, 178, 176, 174, 172, 169, 167, 165, 162, 160, 158, 155, 154, 151, 150, 147, 145, 143, 140,
        139, 136, 135, 132, 131, 128, 127, 124, 123, 120, 119, 116, 115, 112, 111, 108, 107, 104, 103, 102, 99, 98, 95, 94,
        93, 90, 89, 87, 85, 84, 81, 80, 79, 76, 75, 73, 71, 70, 68, 67, 65, 63, 62, 60, 58, 57, 56, 53, 52, 51, 50, 47, 46,
        45, 43, 42, 40, 39, 37, 36, 35, 32, 31, 30, 29, 28, 26, 24, 23, 21, 20, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
        19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
        19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 21, 23, 24, 26, 28, 29, 30,
        31, 32, 35, 36, 37, 39, 40, 42, 43, 45, 46, 47, 50, 51, 52, 53, 56, 57, 58, 60, 62, 63, 65, 67, 68, 70, 71, 73, 75,
        76, 79, 80, 81, 84, 85, 87, 89, 90, 93, 94, 95, 98, 99, 102, 103, 104, 107, 108, 111, 112, 115, 116, 119, 120, 123,
        124, 127, 128, 131, 132, 135, 136, 139, 140, 143, 145, 147, 150, 151, 154, 155, 158, 160, 162, 165, 167, 169, 172,
        174, 176, 178, 181, 183, 185, 188, 191, 193, 195, 198, 201, 202, 205, 208, 211, 214, 217, 218, 221, 224, 227, 230,
        233, 236, 237, 240, 243, 246, 249, 252, 255, 258, 261, 264, 267, 270, 273, 276, 279, 282, 287, 290, 293, 296, 299,
        302, 305, 309, 313, 316, 320, 323, 328, 331, 334, 337, 342, 345, 348, 353, 357, 360, 365, 368, 371, 376, 380, 385,
        388, 393, 396, 402, 405, 410, 413, 419, 422, 427, 432, 436, 441, 446, 450, 455, 461, 464, 470, 475, 480, 484, 489,
        495, 500, 506, 511, 517, 523, 528, 534, 539, 545, 551, 557, 562, 568, 574, 580, 586, 593, 599, 605, 611, 619, 625,
        631, 639, 646, 652, 660, 666, 674, 681, 689, 695, 704, 712, 719, 727, 736, 743, 751, 760, 769, 776, 785, 794, 803,
        812, 821, 830, 839, 848, 858, 867, 879, 889, 898, 908, 920, 930, 939, 952, 962, 974, 984, 997, 1007, 1020, 1033,
        1046, 1056, 1069, 1082, 1096, 1109, 1123, 1136, 1150, 1164, 1178, 1192, 1209, 1223, 1237, 1254, 1269, 1283, 1301,
        1319, 1334, 1352, 1370, 1388, 1404, 1422, 1441, 1460, 1479, 1502, 1521, 1541, 1560, 1583, 1604, 1627, 1647, 1671,
        1696, 1717, 1741, 1766, 1791, 1816, 1842, 1871, 1897, 1924, 1954, 1981, 2011, 2043, 2070, 2102, 2134, 2166, 2199,
        2236, 2269, 2303, 2340, 2377, 2410, 2448, 2485, 2522, 2560, 2597, 2638, 2676, 2717, 2754, 2796, 2837, 2879, 2920,
        2966, 3007, 3053, 3095, 3140, 3186, 3231, 3281, 3327, 3376, 3422, 3472, 3522, 3576, 3625, 3675, 3729, 3783, 3837,
        3891, 3949, 4007, 4061, 4123, 4181, 4239, 4301, 4363, 4426, 4488, 4554, 4620, 4687, 4757, 4824, 4894, 4965, 5039,
        5110, 5189, 5263, 5342, 5421, 5500, 5582, 5665, 5748, 5835, 5923, 6010, 6101, 6192, 6287, 6383, 6482, 6582, 6681,
        6785, 6889, 6996, 7108, 7220, 7337, 7453, 7569, 7693, 7818, 7942, 8075, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191
    }};

    disp_pt_lut_ = {{
        0, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 7956, 7730,
        7515, 7311, 7116, 6932, 6754, 6587, 6425, 6272, 6125, 5983, 5848, 5718, 5593, 5473, 5358, 5247, 5139, 5036, 4936,
        4840, 4747, 4657, 4569, 4485, 4404, 4325, 4249, 4174, 4103, 4032, 3964, 3899, 3834, 3772, 3712, 3652, 3595, 3539,
        3484, 3432, 3380, 3330, 3280, 3233, 3186, 3141, 3096, 3052, 3010, 2968, 2927, 2888, 2849, 2810, 2774, 2737, 2701,
        2667, 2633, 2599, 2566, 2533, 2502, 2471, 2441, 2412, 2383, 2354, 2326, 2298, 2271, 2244, 2219, 2193, 2168, 2143,
        2119, 2095, 2072, 2049, 2026, 2004, 1981, 1960, 1938, 1917, 1897, 1877, 1857, 1837, 1818, 1798, 1780, 1762, 1744,
        1726, 1709, 1692, 1673, 1658, 1641, 1624, 1608, 1592, 1576, 1560, 1546, 1531, 1515, 1501, 1486, 1473, 1458, 1445,
        1430, 1417, 1404, 1390, 1378, 1365, 1353, 1339, 1327, 1315, 1303, 1291, 1280, 1268, 1257, 1245, 1234, 1223, 1212,
        1201, 1190, 1180, 1169, 1158, 1149, 1139, 1128, 1118, 1108, 1099, 1089, 1081, 1071, 1061, 1053, 1043, 1034, 1026,
        1016, 1008, 999, 991, 982, 974, 966, 958, 949, 942, 934, 926, 918, 910, 903, 896, 887, 880, 873, 866, 858, 852, 845,
        838, 830, 824, 817, 810, 804, 798, 790, 784, 778, 771, 765, 759, 753, 747, 740, 734, 728, 722, 716, 710, 705, 699,
        693, 687, 682, 676, 671, 665, 660, 654, 649, 644, 638, 634, 629, 624, 618, 613, 608, 603, 598, 593, 589, 584, 579,
        574, 569, 566, 561, 556, 551, 547, 542, 538, 534, 529, 524, 521, 516, 512, 507, 504, 499, 495, 491, 487, 483, 479,
        474, 471, 467, 463, 459, 455, 451, 448, 444, 440, 437, 433, 429, 426, 422, 419, 415, 411, 408, 404, 400, 397, 393,
        391, 387, 383, 380, 377, 374, 370, 368, 364, 360, 358, 354, 351, 348, 344, 342, 338, 336, 332, 330, 326, 324, 320,
        318, 314, 312, 310, 306, 304, 301, 298, 295, 293, 289, 287, 285, 282, 279, 276, 274, 272, 268, 266, 263, 261, 258,
        256, 254, 251, 248, 245, 243, 241, 238, 236, 234, 231, 229, 226, 224, 222, 219, 217, 215, 212, 211, 209, 206, 204,
        202, 199, 197, 196, 193, 191, 189, 186, 184, 183, 180, 178, 175, 174, 172, 170, 167, 166, 164, 161, 160, 158, 155,
        154, 152, 149, 148, 146, 143, 142, 140, 138, 136, 135, 132, 130, 129, 126, 125, 123, 122, 119, 118, 116, 114, 112,
        111, 108, 107, 105, 104, 101, 100, 97, 96, 95, 93, 91, 89, 88, 86, 84, 83, 82, 79, 78, 75, 74, 73, 71, 69, 68, 66,
        64, 63, 62, 59, 58, 57, 55, 53, 52, 51, 48, 47, 46, 45, 42, 41, 40, 38, 37, 35, 33, 32, 31, 30, 27, 26, 25, 23, 22,
        21, 18, 17, 16, 15, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
        13, 13, 13, 13, 13, 13, 13, 13, 15, 16, 17, 18, 21, 22, 23, 25, 26, 27, 30, 31, 32, 33, 35, 37, 38, 40, 41, 42, 45,
        46, 47, 48, 51, 52, 53, 55, 57, 58, 59, 62, 63, 64, 66, 68, 69, 71, 73, 74, 75, 78, 79, 82, 83, 84, 86, 88, 89, 91,
        93, 95, 96, 97, 100, 101, 104, 105, 107, 108, 111, 112, 114, 116, 118, 119, 122, 123, 125, 126, 129, 130, 132, 135,
        136, 138, 140, 142, 143, 146, 148, 149, 152, 154, 155, 158, 160, 161, 164, 166, 167, 170, 172, 174, 175, 178, 180,
        183, 184, 186, 189, 191, 193, 196, 197, 199, 202, 204, 206, 209, 211, 212, 215, 217, 219, 222, 224, 226, 229, 231,
        234, 236, 238, 241, 243, 245, 248, 251, 254, 256, 258, 261, 263, 266, 268, 272, 274, 276, 279, 282, 285, 287, 289,
        293, 295, 298, 301, 304, 306, 310, 312, 314, 318, 320, 324, 326, 330, 332, 336, 338, 342, 344, 348, 351, 354, 358,
        360, 364, 368, 370, 374, 377, 380, 383, 387, 391, 393, 397, 400, 404, 408, 411, 415, 419, 422, 426, 429, 433, 437,
        440, 444, 448, 451, 455, 459, 463, 467, 471, 474, 479, 483, 487, 491, 495, 499, 504, 507, 512, 516, 521, 524, 529,
        534, 538, 542, 547, 551, 556, 561, 566, 569, 574, 579, 584, 589, 593, 598, 603, 608, 613, 618, 624, 629, 634, 638,
        644, 649, 654, 660, 665, 671, 676, 682, 687, 693, 699, 705, 710, 716, 722, 728, 734, 740, 747, 753, 759, 765, 771,
        778, 784, 790, 798, 804, 810, 817, 824, 830, 838, 845, 852, 858, 866, 873, 880, 887, 896, 903, 910, 918, 926, 934,
        942, 949, 958, 966, 974, 982, 991, 999, 1008, 1016, 1026, 1034, 1043, 1053, 1061, 1071, 1081, 1089, 1099, 1108,
        1118, 1128, 1139, 1149, 1158, 1169, 1180, 1190, 1201, 1212, 1223, 1234, 1245, 1257, 1268, 1280, 1291, 1303, 1315,
        1327, 1339, 1353, 1365, 1378, 1390, 1404, 1417, 1430, 1445, 1458, 1473, 1486, 1501, 1515, 1531, 1546, 1560, 1576,
        1592, 1608, 1624, 1641, 1658, 1673, 1692, 1709, 1726, 1744, 1762, 1780, 1798, 1818, 1837, 1857, 1877, 1897, 1917,
        1938, 1960, 1981, 2004, 2026, 2049, 2072, 2095, 2119, 2143, 2168, 2193, 2219, 2244, 2271, 2298, 2326, 2354, 2383,
        2412, 2441, 2471, 2502, 2533, 2566, 2599, 2633, 2667, 2701, 2737, 2774, 2810, 2849, 2888, 2927, 2968, 3010, 3052,
        3096, 3141, 3186, 3233, 3280, 3330, 3380, 3432, 3484, 3539, 3595, 3652, 3712, 3772, 3834, 3899, 3964, 4032, 4103,
        4174, 4249, 4325, 4404, 4485, 4569, 4657, 4747, 4840, 4936, 5036, 5139, 5247, 5358, 5473, 5593, 5718, 5848, 5983,
        6125, 6272, 6425, 6587, 6754, 6932, 7116, 7311, 7515, 7730, 7956, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191,
        8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191, 8191
    }};
    
    rels_lut_ = {{
        64, 64, 64, 65, 65, 65, 66, 66, 66, 67, 67, 68, 68, 68, 69, 69, 69, 70, 70, 70, 71, 71, 71, 72, 72, 73, 73, 73, 74,
        74, 74, 75, 75, 75, 76, 76, 76, 77, 77, 78, 78, 78, 79, 79, 79, 80, 80, 80, 81, 81, 81, 82, 82, 82, 83, 83, 83, 84,
        84, 84, 85, 85, 85, 86, 86, 86, 87, 87, 87, 87, 88, 88, 88, 89, 89, 89, 90, 90, 90, 91, 91, 91, 91, 92, 92, 92, 93,
        93, 93, 93, 94, 94, 94, 95, 95, 95, 95, 96, 96, 96, 96, 97, 97, 97, 98, 98, 98, 98, 99, 99, 99, 99, 100, 100, 100,
        100, 101, 101, 101, 101, 101, 102, 102, 102, 102, 103, 103, 103, 103, 104, 104, 104, 104, 104, 105, 105, 105, 105,
        105, 106, 106, 106, 106, 106, 107, 107, 107, 107, 107, 108, 108, 108, 108, 108, 109, 109, 109, 109, 109, 109, 110,
        110, 110, 110, 110, 110, 111, 111, 111, 111, 111, 111, 112, 112, 112, 112, 112, 112, 112, 113, 113, 113, 113, 113,
        113, 113, 114, 114, 114, 114, 114, 114, 114, 115, 115, 115, 115, 115, 115, 115, 115, 116, 116, 116, 116, 116, 116,
        116, 116, 116, 117, 117, 117, 117, 117, 117, 117, 117, 117, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118, 118,
        119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 119, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120,
        120, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 122, 122, 122, 122, 122, 122,
        122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123,
        123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124,
        124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 125, 125, 125, 125, 125,
        125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125,
        125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 126, 126, 126, 126, 126, 126,
        126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126,
        126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126,
        126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126,
        126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 127,
        127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
        127, 127, 127, 127, 127, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10,
        10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 14, 14,
        14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18,
        18, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 24, 24, 24,
        24, 25, 25, 25, 25, 26, 26, 26, 26, 26, 27, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 29, 30, 30, 30, 31, 31, 31, 31,
        32, 32, 32, 32, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40,
        40, 40, 41, 41, 41, 42, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45, 45, 46, 46, 46, 47, 47, 47, 48, 48, 48, 49, 49, 49,
        50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 53, 54, 54, 54, 55, 55, 56, 56, 56, 57, 57, 57, 58, 58, 58, 59, 59, 59, 60,
        60, 61, 61, 61, 62, 62, 62, 63, 63
    }};

    dxy_lut_ = {{
        0, 0, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11,
        11, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 21, 21, 21, 21, 22,
        22, 22, 22, 24, 24, 24, 24, 25, 25, 25, 25, 27, 27, 27, 28, 28, 28, 28, 30, 30, 30, 30, 31, 31, 31, 31, 33, 33, 33,
        33, 35, 35, 35, 35, 36, 36, 36, 36, 38, 38, 38, 40, 40, 40, 40, 41, 41, 41, 41, 43, 43, 43, 43, 45, 45, 45, 45, 47,
        47, 47, 47, 49, 49, 49, 51, 51, 51, 51, 52, 52, 52, 52, 54, 54, 54, 54, 56, 56, 56, 56, 58, 58, 58, 58, 60, 60, 60,
        60, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,
        63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64,
        -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -64, -62,
        -62, -62, -60, -60, -60, -60, -58, -58, -58, -58, -56, -56, -56, -56, -54, -54, -54, -54, -52, -52, -52, -52, -51,
        -51, -51, -51, -49, -49, -49, -47, -47, -47, -47, -45, -45, -45, -45, -43, -43, -43, -43, -41, -41, -41, -41, -40,
        -40, -40, -40, -38, -38, -38, -36, -36, -36, -36, -35, -35, -35, -35, -33, -33, -33, -33, -31, -31, -31, -31, -30,
        -30, -30, -30, -28, -28, -28, -28, -27, -27, -27, -25, -25, -25, -25, -24, -24, -24, -24, -22, -22, -22, -22, -21,
        -21, -21, -21, -19, -19, -19, -19, -18, -18, -18, -18, -17, -17, -17, -15, -15, -15, -15, -14, -14, -14, -14, -13,
        -13, -13, -13, -11, -11, -11, -11, -10, -10, -10, -10, -9, -9, -9, -8, -8, -8, -8, -7, -7, -7, -7, -6, -6, -6, -6,
        -4, -4, -4, -4, -3, -3, -3, -3, -2, -2, -2, -2, 0
    }};
}

ActivationLut::~ActivationLut() {
    // Do Nothing
}

void ActivationLut::update(
        const edm::Event&,
        const edm::EventSetup&
) {
    // Do Nothing
}

const trk_pt_t& ActivationLut::lookup_prompt_pt(const trk_nn_address_t& address) const {
    ap_uint<10> bin = address.to_string(AP_HEX).c_str();
    return prompt_pt_lut_[bin];
}

const trk_pt_t& ActivationLut::lookup_disp_pt(const trk_nn_address_t& address) const {
    ap_uint<10> bin = address.to_string(AP_HEX).c_str();
    return disp_pt_lut_[bin];
}

const trk_rels_t& ActivationLut::lookup_rels(const trk_nn_address_t& address) const {
    ap_uint<10> bin = address.to_string(AP_HEX).c_str();
    return rels_lut_[bin];
}

const trk_dxy_t& ActivationLut::lookup_dxy(const trk_nn_address_t& address) const {
    ap_uint<10> bin = address.to_string(AP_HEX).c_str();
    return dxy_lut_[bin];
}

