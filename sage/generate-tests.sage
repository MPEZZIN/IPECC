#!/usr/bin/env sage

#
#  Copyright (C) 2023 - This file is part of IPECC project
#
#  Authors:
#      Karim KHALFALLAH <karim.khalfallah@ssi.gouv.fr>
#      Ryad BENADJILA <ryadbenadjila@gmail.com>
#
#  Contributors:
#      Adrian THILLARD
#      Emmanuel PROUFF
#
#  This software is licensed under GPL v2 license.
#  See LICENSE file at the root folder of the project.
#

import random
import sys
from os.path import getsize

def toss_a_coin():
	coin = random.randint(1, 2)
	if coin == 1:
		return 1
	else:
		return 0

# To generate a safe prime.
def rdp(nbits=256):
	while True:
		p = random_prime(2^nbits-1, false, 2^(nbits-1))
		if ZZ((p-1)/2).is_prime():
			return p


######################### C O N F I G U R A T I O N ############################
#                                                                              #
#              (You should only edit parameters within this frame)             #
#                                                                              #
# Parameter: 'ww' ##############################################################
#                                                                              #
ww = 16                                                                        #
#                                                                              #
#   Designates the bit-width of limbs used for the representation of large     #
#   numbers inside IPECC's internal memory.                                    #
#                                                                              #
#   It is only used to define 'nnmin' parameter (see a few lines below)        #
#   which defines the smallest value admissible by an actual hardware imple-   #
#   mentation of IPECC.  Hence the value of 'ww' only matters if the present   #
#   script is used to generate test-vectors *for the actual hardware to run*,  #
#   and if that hardware was synthesized with option 'nn_dynamic' = TRUE in    #
#   ecc_customize.vhd (to enforce that no test is generated with a dynamic     #
#   value of 'nn' that goes beyond 'nnmin').                                   #
#                                                                              #
#   On the other hand, if the hardware you want to submit the tests to was     #
#   synthesized with option 'nn_dynnamic' = FALSE, then simply ignore para-    #
#   meters 'ww', 'nnmin', 'nnmax' and simply set 'nn_constant' to the static   #
#   value that was also set for parameter 'nn' in ecc_customize.vhd.           #
#                                                                              #
#   Alternatively, you can ignore value of 'ww' and directly set a numerical   #
#   value to 'nnmin', but do keep in mind that any particular hardware imple-  #
#   mentation of IPECC will only be able to perform computations with a mini-  #
#   mal value of 'nn' which depends on the value of 'ww', which is precisely   #
#   'ww - 4 + 1', hence the default value set for parameter 'nnmin' below.     #
#                                                                              #
#   For any particular hardware implementation of IPECC, the value of 'ww' is  #
#   automatically set at synthesis time based on the technology that was       #
#   given in file ecc_customize.vhd:                                           #
#                                                                              #
#     - on FPGA targets, value of 'ww' is set based on the device/family       #
#     - on ASIC targets, value of 'ww' is the same as the value of parameter   #
#       'multwidth'.                                                           #
#                                                                              #
#   Please see ecc_customize.vhd in-file documentation for more information.   #
#                                                                              #
#   On 7-series/Zynq Xilinx FPGAs, 'ww' is set to 16.                          #
#                                                                              #
#                                                                              #
# Parameter: 'nnmin' ###########################################################
#                                                                              #
#nnmin = ww - 3                                                                #
nnmin = 32                                                                     #
#                                                                              #
#   Also read info on parameter 'ww' above.                                    #
#                                                                              #
#   The smallest admissible value for 'nn' for any particular hardware imple-  #
#   mentation of IPECC ensures that w >= 2 where w = ceil((nn + 4) / ww) which #
#   is equivalent to ( (nn + 4) / ww ) > 1 and therefore nn > ww - 4 which     #
#   gives the minimum ww - 3 set above.                                        #
#                                                                              #
#       HOWEVER: there's a known BUG in the [k]P computation of IPECC for very #
#       small curve sizes. This bug's been characterized and should be fixed   #
#       soon. It comes from the fact that driver rounds the value of 'nn' that #
#       it sets in the hardware up to the next byte (e.g if a curve has nn=13, #
#       then the driver will set 16). Now this creates the possibility that    #
#       random masks generated for the "Z-remask" countermeasure happen to be  #
#       a multiple of p, thus corrupting points coordinates (as these randoms  #
#       are used for multiplicative masking). On Xilinx FPGAs, ww = 16  so     #
#       ww - 3 makes it a very small 13, a value for which the bug will        #
#       present itself very quick, within the first thousands of [k]P computa- #
#       tions submitted to the IP.                                             #
#                                                                              #
#   This is why for now 'nnmin' is set to 32 instead of ww - 3 (=13 in Xilinx) #
#                                                                              #
#                                                                              #
# Parameters: 'nnmax', 'nnminmax', 'nnmaxabsolute'                             #
#             'NNMINMOD', 'NNMININCR', 'NNMAXMOD', 'NNMAXINCR' #################
#                                                                              #
nnmax = nnmin + 16     # For start (will increase and plateau to absolute max) #
nnmaxabsolute = 256    # Largest possible value of 'nn'.                       #
nnminmax = 256         # Largest possible value of 'nnmin'.                    #
NNMINMOD = 10          # 'nnmin' will increase every 'NNMINMOD' curves,        #
NNMININCR = 32         #  and it will be increased by 'NNMININCR'.             #
NNMAXMOD = 5           # 'nnmax' will increase every 'NNMAXMOD' curves,        #
NNMAXINCR = 32         #  and it will be increased by 'NNMAXINCR'.             #
#                                                                              #
#   This script generates test-vectors by gradually increasing the range from  #
#   which the random values of 'nn' are withdrawn for each new curve, this     #
#   range being defined by [nnmin : nnmax].                                    #
#                                                                              #
#   Both 'nnmin' and 'nnmax' are regularly increased:                          #
#                                                                              #
#     - parameter 'nnmin' is increased every period of 'NNMINMOD' generated    #
#       curves (default value of 'NNMINMOD' is 200) and by an increment of     #
#       'NNMININCR' (default value is 1)                                       #
#                                                                              #
#     - parameter 'nnmax' is increased every period of 'NNMAXMOD' generated    #
#       curves (default value is 100) and by an increment of 'NNMAXINCR'       #
#       (default value is 3).                                                  #
#                                                                              #
#   Now, as 'nnmin' is regularly increased, parameter 'nnminmax' defines the   #
#   maximal threeshold that 'nnmin' will never exceed.                         #
#                                                                              #
#   Similarly, 'nnmaxabsolute' defines the maximal threeshold that 'nnmax'     #
#   will neither ever exceed, while 'nnmax' being initially set at nnmin + 16  #
#   is actually quite arbitrary.                                               #
#                                                                              #
#   Warning: keep NNMAXMOD < NNMINMOD to avoid range inversion error!          #
#                                                                              #
# Parameters: 'nn_constant', 'only_kp_and_no_blinding' #########################
#                                                                              #
nn_constant = 0  # Non-0 value will make it the constant unique value of 'nn', #
#                # hence bypassing previous settings for nnmin, nnmax, etc.    #
#                                                                              #
only_kp_and_no_blinding = False  # Hope that option's name speaks for itself   #
#                                # (if true, this script will generate test-   #
#                                # vectors for [k]P operation only, discarding #
#                                # point addition, point doubling, point equa- #
#                                # lity/opposition tests, etc. also blinding   #
#                                # will be kept disabled).                     #
#                                                                              #
# Parameters: 'NBCURV'                                                         #
#             'NB*' where * = KP|ADD|DBL|NEG|CHK|EQU|OPP #######################
#                                                                              #
NBCURV = 0 # A value of 0 means don't stop (ever-lasting producing loop).      #
NBKP = 100  # Nb of [k]P tests that will be generated per curve.               #
NBADD = 10 # Nb of P+Q tests that will be generated per curve.                 #
NBDBL = 10 # Nb of [2]P tests that will be generated per curve.                #
NBNEG = 10 # Nb of (-P) tests that will be generated per curve.                #
NBCHK = 10 # Nb of 'is point on curve?' tests that will be generated per curve #
NBEQU = 10 # Nb of 'are points equal?' tests that will be generated per curve. #
NBOPP = 10 # Nb of 'are points opposite?" tests that'll be generated per curve #
#                                                                              #
#   For any new curve, a random value is drawn from the current range          #
#   [nnmin : nnmax], and then 6 six types of tests are generated for that      #
#   curve:                                                                     #
#                                                                              #
#     - NBKP defines the number of [k]P tests.                                 #
#                                                                              #
#     - NBADD defines the number of P + Q tests. Points are also generated     #
#       randomly.                                                              #
#                                                                              #
#     - NBDBL defines the number of [2]P tests (computation of the double      #
#       of a point.                                                            #
#                                                                              #
#     - NBNEG defines the number of (-P) tests (computation of the opposite    #
#       of a point).                                                           #
#                                                                              #
#     - NBCHK, NBEQU and NBOPP resp. define the number of boolean 'is point    #
#       on curve?' tests, 'are points equal' tests, and 'are points opposi-    #
#       te?' tests, resp. Some of the tests have their answer set to TRUE,     #
#       some deliberately to FALSE.                                            #
#                                                                              #
# All points involved in the tests are generated at random (using SageMath's   #
# random_element() method on the elliptic curve opject type). Also the scalar  #
# and any other parmaters such as the cruve parameters a, b, p and q are also  #
# generated at random.                                                         #
#                                                                              #
# The complete script will iterate on a total number of 'NBCURV' curves.       #
# Setting 0 to 'NBCURV' means the loop shouldn't stop.                         #
#                                                                              #
# Parameter 'NN_LIMIT_COMPUTE_Q' ###############################################
#                                                                              #
NN_LIMIT_COMPUTE_Q = 64  # Limit of 'nn' above which blinding will be disabl.  #
#                                                                              #
#   For [k]P tests, blinding may or may not be enabled (and if so, with a      #
#   number of blinding bits randomly drawn in the range [1 : nn - 1]).         #
#   Generating a test with blinding enabled requires first to compute the      #
#   order of the curve, which can become insupportably long as value of 'nn'   #
#   exceeds some threeshold. Such a threeshold is difficult to assess,         #
#   however that's the reason for parameter 'NN_LIMIT_COMPUTE_Q'.              #
#   By definition, any random curve generated for a value of 'nn' exceeding    #
#   'NN_LIMIT_COMPUTE_Q' will involve no blinding in their [k]P tests          #
#   generation. Furthermore, for these curves, the order 'q' will be           #
#   set to the large number 1, as an artifact which can't make no harm         #
#   as 'q' only plays a role in IPECC when blinding countermeasure is          #
#   enabled in a [k]P computation.                                             #
#                                                                              #
# Parameter: 'NO_EXCEPTIONS' ###################################################
#                                                                              #
NO_EXCEPTIONS = False                                                          #
#                                                                              #
#   If set to False, the script will also generate, for each generated         #
#   curve and in addition to the tests defined above (c.f NBKP, NBADD, etc)    #
#   a certain number of tests that will have the IP to meet an exception       #
#   during computation, like for instance adding two points which are          #
#   opposite, or multiplying a point by a scalar equal to its order, etc.      #
#                                                                              #
#   If set to True, no such test will be generated by the script.              #
#                                                                              #
# Note #########################################################################
#                                                                              #
#   Obviously what is interesting in cryptographic applications is to be       #
#   able to perform computations on numbers of... cryptographic sizes.         #
#                                                                              #
#   You may then find testing small values like nn=32 to be be inappro-        #
#   priate, however this is not completely true, as these tests can be         #
#   performed faster, and correlatively in much higher quantity (also          #
#   think about HDL testbenchs which are dramatically slow) enforcing          #
#   verification of pure control aspects of the computations carried           #
#   inside the IP.                                                             #
#                                                                              #
#   This is also the reason for the [nnmin : nnmax] range described above:     #
#   tests will start quite fast at the begining of one test campaign,          #
#   then become much slower as values of nnmin & nnmax increase.               #
#                                                                              #
################################################################################

nbcurv = 0
nbtest = 0

KNRM="\x1B[0m"
KRED="\x1B[31m"
KYEL="\x1B[33m"
KWHT="\x1B[37m"

def div(i, s):
    if ((i % s) == 0):
        return (i // s)
    else:
        return (i // s) + 1;

if nn_constant == 0:
    sys.stderr.write(KWHT + "Generating curves from nn = " + str(nnmin) + " to " + str(nnmax) + KNRM + "\n")
else:
    sys.stderr.write(KWHT + "Generating curves for nn = " + str(nn_constant) + KNRM + "\n")

# infinite loop
while (nbcurv < NBCURV) or (NBCURV == 0):
    if (nn_constant != 0):
        nn = nn_constant
    else:
        new_min_or_max = False
        if (nbcurv % NNMAXMOD) == NNMAXMOD - 1:
            prev_max = nnmax
            nnmax = nnmax + NNMAXINCR;
            if nnmax > nnmaxabsolute:
                nnmax = nnmaxabsolute
            new_min_or_max = True
        if (nbcurv % NNMINMOD) == NNMINMOD - 1:
            prev_min = nnmin
            nnmin = nnmin + NNMININCR;
            if nnmin > nnminmax:
                nnmin = nnminmax
            new_min_or_max = True
        if new_min_or_max and (nnmax != prev_max or nnmin != prev_min):
            sys.stderr.write(KWHT + "Generating curves from nn = "
                    + str(nnmin) + " to " + str(nnmax) + KNRM + "\n")
        # generate a random prime size (nn)
        nn = random.randint(nnmin, nnmax)
    # generate a random prime (p)
    while True:
        p = rdp(nn)
        if (is_prime(p) == True):
            break
    # algebraic definitions, field & curve
    Fp = GF(p)
    disc = 0
    while disc == 0:
        # generate value of a
        a = Fp.random_element()
        # generate value of b
        b = Fp.random_element()
        # check curve discriminant condition
        disc = -16 * ( (4 * (a**3)) + (27 * (b**2)) )
    EE = EllipticCurve(Fp, [a,b])
    # compute value of q (order of the curve)
    #   but only if nn < 256 (or equal) otherwise Sage computation is
    #   too long (only if we do compute q do we also generate tests with
    #   blinding)
    if nn > NN_LIMIT_COMPUTE_Q:
        q = 1
    else:
        q = EE.order()
    # nn might need to be adjusted to get into account the size of q
    # as nn must be equal to max(log2(p), log2(q))
    nn = max(nn, ceil(RR(log(q, 2))))
    if (nn > nnmaxabsolute):
        sys.stderr.write("met size of q > nnmaxabsolute\n")
        continue
    # print (on standard output) the algebraic & curve parameters
    print("== NEW CURVE #" + str(nbcurv))
    print("nn=" + str(nn))
    print("p=0x%0*x" % (int(div(nn, 4)), p))
    print("a=0x%0*x" % (int(div(nn, 4)), a))
    print("b=0x%0*x" % (int(div(nn, 4)), b))
    print("q=0x%0*x" % (int(div(nn, 4)), q))
    # #############################################################
    #                  REGULAR TESTS (NO EXCEPTION)
    # #############################################################
    #
    # TEST : [k]P computation
    #
    for i in range(0, NBKP):
        # generate a random point on curve
        P = EE.random_element()
        xP = P[0]
        yP = P[1]
        # generate random value of scalar
        k = Integer(random.randint(0, (2**nn) - 1))
        # compute [k]P
        kP = k * P
        # print test informations
        print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
        if P == 0:
            print("P=0")
        else:
            print("Px=0x%0*x" % (int(div(nn, 4)), xP))
            print("Py=0x%0*x" % (int(div(nn, 4)), yP))
        print("k=0x%0*x" % (int(div(nn, 4)), k))
        if not only_kp_and_no_blinding:
            if nn <= NN_LIMIT_COMPUTE_Q:
                if toss_a_coin() == 1:
                    nbbld = random.randint(1, nn - 1)
                    print("nbbld=%d" % nbbld)
        if (kP != 0):
            print("kPx=0x%0*x" % (int(div(nn, 4)), Integer(kP[0])))
            print("kPy=0x%0*x" % (int(div(nn, 4)), Integer(kP[1])))
        else:
            print("kP=0")
        nbtest+=1
    if only_kp_and_no_blinding:
        nbcurv+=1
        continue
    #
    # TEST : P + Q
    #
    for i in range(0, NBADD):
        # generate a random point on curve
        P = EE.random_element()
        xP = P[0]
        yP = P[1]
        # generate a second random point on curve
        Q = EE.random_element()
        xQ = Q[0]
        yQ = Q[1]
        # compute P + Q
        PplusQ = P + Q
        # print test informations
        print("== TEST P+Q #%d.%d" % (nbcurv, nbtest))
        if P == 0:
            print("P=0")
        else:
            print("Px=0x%0*x" % (int(div(nn, 4)), xP))
            print("Py=0x%0*x" % (int(div(nn, 4)), yP))
        if Q == 0:
            print("Q=0")
        else:
            print("Qx=0x%0*x" % (int(div(nn, 4)), xQ))
            print("Qy=0x%0*x" % (int(div(nn, 4)), yQ))
        if (PplusQ == 0):
            print("PplusQ=0")
        else:
            print("PplusQx=0x%0*x" % (int(div(nn, 4)), Integer(PplusQ[0])))
            print("PplusQy=0x%0*x" % (int(div(nn, 4)), Integer(PplusQ[1])))
        nbtest+=1
    #
    # TEST : [2]P
    #
    for i in range(0, NBDBL):
        # generate a random point on curve
        P = EE.random_element()
        xP = P[0]
        yP = P[1]
        # compute [2]P
        twoP = 2 * P
        # print test informations
        print("== TEST [2]P #%d.%d" % (nbcurv, nbtest))
        if P == 0:
            print("P=0")
        else:
            print("Px=0x%0*x" % (int(div(nn, 4)), xP))
            print("Py=0x%0*x" % (int(div(nn, 4)), yP))
        if (twoP == 0):
            print("twoP=0")
        else:
            print("twoPx=0x%0*x" % (int(div(nn, 4)), Integer(twoP[0])))
            print("twoPy=0x%0*x" % (int(div(nn, 4)), Integer(twoP[1])))
        nbtest+=1
    #
    # TEST : -P
    #
    for i in range(0, NBNEG):
        # generate a random point on curve
        P = EE.random_element()
        xP = P[0]
        yP = P[1]
        # compute -P
        negP = -P
        # print test informations
        print("== TEST -P #%d.%d" % (nbcurv, nbtest))
        if P == 0:
            print("P=0")
        else:
            print("Px=0x%0*x" % (int(div(nn, 4)), xP))
            print("Py=0x%0*x" % (int(div(nn, 4)), yP))
        if (negP == 0):
            print("negP=0")
        else:
            print("negPx=0x%0*x" % (int(div(nn, 4)), Integer(negP[0])))
            print("negPy=0x%0*x" % (int(div(nn, 4)), Integer(negP[1])))
        nbtest+=1
    #
    # TEST : is P on curve
    #
    for i in range(0, NBCHK):
        print("== TEST isPoncurve #%d.%d" % (nbcurv, nbtest))
        if toss_a_coin() == 1:
            # generate a random point on curve
            P = EE.random_element()
            xP = P[0]
            yP = P[1]
            # print test informations
            if P == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), xP))
                print("Py=0x%0*x" % (int(div(nn, 4)), yP))
            print("true")
        else:
            # create a false point (one that is not on curve)
            xP = Fp.random_element()
            yP = Fp.random_element()
            print("Px=0x%0*x" % (int(div(nn, 4)), xP))
            print("Py=0x%0*x" % (int(div(nn, 4)), yP))
            # check that the 2-uple (xP, yP) is not a point
            if (yP**2) == (xP**3) + (a * xP) + b:
                # P can't be the null point
                print("true")
            else:
                print("false")
        nbtest+=1
    #
    # TEST : P == Q
    #
    for i in range(0, NBEQU):
        print("== TEST isP==Q #%d.%d" % (nbcurv, nbtest))
        if toss_a_coin() == 1:
            # generate a random point on curve
            P = EE.random_element()
            xP = P[0]
            yP = P[1]
            # generate a second random point on curve
            Q = EE.random_element()
            xQ = Q[0]
            yQ = Q[1]
            if P == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), xP))
                print("Py=0x%0*x" % (int(div(nn, 4)), yP))
            if Q == 0:
                print("Q=0")
            else:
                print("Qx=0x%0*x" % (int(div(nn, 4)), xQ))
                print("Qy=0x%0*x" % (int(div(nn, 4)), yQ))
            if (P == Q):
                print("true")
            else:
                print("false")
        else:
            # generate a random point on curve
            P = EE.random_element()
            xP = P[0]
            yP = P[1]
            if P == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), xP))
                print("Py=0x%0*x" % (int(div(nn, 4)), yP))
            if P == 0:
                print("Q=0")
            else:
                print("Qx=0x%0*x" % (int(div(nn, 4)), xP))
                print("Qy=0x%0*x" % (int(div(nn, 4)), yP))
            print("true")
        nbtest+=1
    #
    # TEST : P == -Q
    #
    for i in range(0, NBEQU):
        print("== TEST isP==-Q #%d.%d" % (nbcurv, nbtest))
        if toss_a_coin() == 1:
            # generate a random point on curve
            P = EE.random_element()
            xP = P[0]
            yP = P[1]
            # generate a second random point on curve
            Q = EE.random_element()
            xQ = Q[0]
            yQ = Q[1]
            if P == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), xP))
                print("Py=0x%0*x" % (int(div(nn, 4)), yP))
            if Q == 0:
                print("Q=0")
            else:
                print("Qx=0x%0*x" % (int(div(nn, 4)), xQ))
                print("Qy=0x%0*x" % (int(div(nn, 4)), yQ))
            if (P == -Q):
                print("true")
            else:
                print("false")
        else:
            # generate a random point on curve
            P = EE.random_element()
            xP = P[0]
            yP = P[1]
            # Compute -P
            mP = -P
            if P == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), xP))
                print("Py=0x%0*x" % (int(div(nn, 4)), yP))
            if P == 0:
                print("Q=0")
            else:
                print("Qx=0x%0*x" % (int(div(nn, 4)), mP[0]))
                print("Qy=0x%0*x" % (int(div(nn, 4)), mP[1]))
            print("true")
        nbtest+=1
    # If no exception test is expected, then bypass all the following
    if NO_EXCEPTIONS:
        nbcurv+=1
        continue
    # #############################################################
    #                    [k]P EXCEPTION TESTS
    # #############################################################
    #
    # Generate a random point on this curve
    P = EE.random_element()
    xP = P[0]
    yP = P[1]
    if nn <= NN_LIMIT_COMPUTE_Q:
        #
        # TEST: [k]P computation with exception: k = q
        #
        k = q
        # compute [k]P
        kP = k * P
        # print test informations
        print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: k = q")
        if P == 0:
            print("P=0")
        else:
            print("Px=0x%0*x" % (int(div(nn, 4)), xP))
            print("Py=0x%0*x" % (int(div(nn, 4)), yP))
        print("k=0x%0*x" % (int(div(nn, 4)), k))
        if nn <= NN_LIMIT_COMPUTE_Q:
            if toss_a_coin() == 1:
                nbbld = random.randint(1, nn - 1)
                print("nbbld=%d" % nbbld)
        # [k]P = 0 necessarily
        print("kP=0")
        nbtest+=1
        #
        # TEST: [k]P computation with exception: k = q + 1
        #
        # we need to test if adding 1 to q will possibly add a bit
        # of dynamic to the scalar as compared to nn - this happens
        # when q is of the form (2**nn - 1) which can happen from
        # time to time on very small random curves
        if (q != (2**nn) - 1):
            k = q + 1
            # compute [k]P
            kP = k * P
            # print test informations
            print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
            print("# EXCEPTION: k = q + 1" )
            if P == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), xP))
                print("Py=0x%0*x" % (int(div(nn, 4)), yP))
            print("k=0x%0*x" % (int(div(nn, 4)), k))
            if nn <= NN_LIMIT_COMPUTE_Q:
                if toss_a_coin() == 1:
                    nbbld = random.randint(1, nn - 1)
                    print("nbbld=%d" % nbbld)
            if kP == 0:
                print("kP=0")
            else:
                print("kPx=0x%0*x" % (int(div(nn, 4)), kP[0]))
                print("kPy=0x%0*x" % (int(div(nn, 4)), kP[1]))
            nbtest+=1
        #
        # TEST: [k]P computation with exception: k = q - 1
        #
        k = q - 1
        # compute [k]P
        kP = k * P
        # print test informations
        print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: k = q - 1" )
        if P == 0:
            print("P=0")
        else:
            print("Px=0x%0*x" % (int(div(nn, 4)), xP))
            print("Py=0x%0*x" % (int(div(nn, 4)), yP))
        print("k=0x%0*x" % (int(div(nn, 4)), k))
        if nn <= NN_LIMIT_COMPUTE_Q:
            if toss_a_coin() == 1:
                nbbld = random.randint(1, nn - 1)
                print("nbbld=%d" % nbbld)
        if kP == 0:
            print("kP=0")
        else:
            print("kPx=0x%0*x" % (int(div(nn, 4)), kP[0]))
            print("kPy=0x%0*x" % (int(div(nn, 4)), kP[1]))
        nbtest+=1
    #
    # TEST: [k]P with exception: k = a factor of P.order()
    #       (implying: result should be the null point)
    #
    if nn <= NN_LIMIT_COMPUTE_Q:
        # compute order of point P
        o = P.order()
        # factor order of P
        facs = list(factor(o))
        # fiter out the cases where order is prime (on the other hand cases where
        # the order is a power of a prime are accepted)
        if len(facs) == 1 and facs[0][1] == 1:
            nbcurv+=1
            continue
        # parse all factors or P's order
        for fac in facs:
            k = fac[0]
            # create the point associated to that factor
            # (it is the point f * P, where f is the product of all factors
            # other than the current considered one)
            fs = 1
            for f in facs:
                if f[0] != fac[0]:
                    fs = fs * (f[0]**f[1])
            if fac[1] > 1:
                fs = fs * (fac[0]**(fac[1] - 1))
            fsP = fs * P
            # print test informations
            print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
            print("# EXCEPTION: k = a factor of P's order")
            if P == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), fsP[0]))
                print("Py=0x%0*x" % (int(div(nn, 4)), fsP[1]))
            # no blinding
            #   (it would taint the test by creating a different scalar,
            #   for which the null point wouldn't be met anymore)
            print("k=0x%0*x" % (int(div(nn, 4)), k))
            # [k]P = 0 necessarily
            print("kP=0")
            nbtest+=1
        #
        # TEST: [k]P with exception: k = a factor of P's order + a multiple
        #       of the next power-of-2
        #       (meaning: point 0 will be met however it shouldn't be the
        #        final result)
        #
        # parse all factors or P's order
        for fac in facs:
            k = fac[0]
            # create the point associated to that factor
            # (it is the point f * P, where f is the product of all factors
            # other than the current considered one)
            fs = 1
            for f in facs:
                if f[0] != fac[0]:
                    fs = fs * (f[0]**f[1])
            if fac[1] > 1:
                fs = fs * (fac[0]**(fac[1] - 1))
            fsP = fs * P
            # form k based on fac[0]
            #   compute nb of bits to encode k
            # print test informations
            print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
            print("# EXCEPTION: k = a factor of P's order + a nb aligned on a " +
                "higher power-of-2")
            nbits_fac = ceil(RR(log(fac[0])/log(2)))
            if nbits_fac == RR(log(fac[0])/log(2)):
                nbits_fac = nbits_fac + 1
            #   generate a random number formed of nn bits - the nb of bits to encode k
            #cpl = Integer(random.randint(2**nbits_fac, (2**nn) - 1))
            cpl = Integer(
                    random.randint(0, (2**(nn - nbits_fac)) - 1)) * (2**nbits_fac)
            print("#    factor = 0x%0*x (%d bits)" % (int(div(nn, 4)),
                Integer(fac[0]), nbits_fac))
            print("#complement = 0x%0*x" % (int(div(nn, 4)), Integer(cpl)))
            k = fac[0] + cpl
            print("#         k = 0x%0*x" % (int(div(nn, 4)), k))
            if fsP == 0:
                print("P=0")
            else:
                print("Px=0x%0*x" % (int(div(nn, 4)), fsP[0]))
                print("Py=0x%0*x" % (int(div(nn, 4)), fsP[1]))
            print("k=0x%0*x" % (int(div(nn, 4)), k))
            # Compute [k]P by Sage
            kP = k * fsP
            if (kP != 0):
                print("kPx=0x%0*x" % (int(div(nn, 4)), Integer(kP[0])))
                print("kPy=0x%0*x" % (int(div(nn, 4)), Integer(kP[1])))
            else:
                print("kP=0")
            nbtest+=1
            #
            # TEST: a second test if the currect factor is a multi-factor
            #
            if (fac[1] > 1) and (fac[0] > 2):
                fs = Integer(fs / (fac[0]**(fac[1] - 1)))
                ff = fac[0]**fac[1]
                fP = fs * P
                #fsP = fs * P
                # form k based on fac[0]
                #   compute nb of bits to encode k
                # print test informations
                print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
                print("# EXCEPTION: k = a factor of P's order + a nb aligned on a " +
                    "higher power-of-2")
                nbits_fac = ceil(RR(log(ff)/log(2)))
                if nbits_fac == RR(log(ff)/log(2)):
                    nbits_fac = nbits_fac + 1
                #   generate a random number of {nn bits - the nb of bits to encode k}
                #cpl = Integer(random.randint(2**nbits_fac, (2**nn) - 1))
                cpl = Integer(
                        random.randint(0, (2**(nn - nbits_fac)) - 1)) * (2**nbits_fac)
                print("#    factor = 0x%0*x (%d bits)" % (int(div(nn, 4)),
                    Integer(ff), nbits_fac))
                print("#complement = 0x%0*x" % (int(div(nn, 4)), Integer(cpl)))
                k = (ff) + cpl
                print("#         k = 0x%0*x" % (int(div(nn, 4)), k))
                if fP == 0:
                    print("P=0")
                else:
                    print("Px=0x%0*x" % (int(div(nn, 4)), fP[0]))
                    print("Py=0x%0*x" % (int(div(nn, 4)), fP[1]))
                print("k=0x%0*x" % (int(div(nn, 4)), k))
                # Compute [k]P by Sage
                kP = k * fP
                if (kP != 0):
                    print("kPx=0x%0*x" % (int(div(nn, 4)), Integer(kP[0])))
                    print("kPy=0x%0*x" % (int(div(nn, 4)), Integer(kP[1])))
                else:
                    print("kP=0")
                nbtest+=1
    dice = random.randint(1, 16)
    if dice == 16:
        #
        # TEST: [k]P with k = 0
        #
        k = 0
        print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: k = 0")
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("k=0x%0*x" % (int(div(nn, 4)), k))
        print("kP=0")
        nbtest+=1
    dice = random.randint(1, 16)
    if dice == 16:
        #
        # TEST: [k]P with P = 0
        #
        k = random.randint(1, 2**(nn - 1))
        print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P = 0")
        print("P=0")
        print("k=0x%0*x" % (int(div(nn, 4)), k))
        print("kP=0")
        nbtest+=1
    dice = random.randint(1, 16)
    if dice == 16:
        #
        # TEST: [k]P with k = 0 and P = 0
        #
        k = 0
        print("== TEST [k]P #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: k = 0 and P = 0")
        print("P=0")
        print("k=0x%0*x" % (int(div(nn, 4)), k))
        print("kP=0")
        nbtest+=1
    # #############################################################
    #         EXCEPTION TESTS ON POINT OPS (OTHER THAN [k]P)
    # #############################################################
    #
    # EXCEPTIONS FOR PT_ADD (P + Q)
    #
    #   P = Q
    if P != 0:
        print("== TEST P+Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P = Q")
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("Qx=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Qy=0x%0*x" % (int(div(nn, 4)), P[1]))
        # have Sage compute P + Q = [2]P here
        twoP = 2 * P
        if twoP == 0:
            print("PplusQx=0")
        else:
            print("PplusQx=0x%0*x" % (int(div(nn, 4)), twoP[0]))
            print("PplusQy=0x%0*x" % (int(div(nn, 4)), twoP[1]))
        nbtest+=1
    #   P = -Q
    if P != 0:
        print("== TEST P+Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P = -Q")
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("Qx=0x%0*x" % (int(div(nn, 4)), (-P)[0]))
        print("Qy=0x%0*x" % (int(div(nn, 4)), (-P)[1]))
        print("PplusQ=0")
        nbtest+=1
    #   P = 0 (Q != 0)
    print("== TEST P+Q #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: P = 0, Q /= 0")
    print("P=0")
    print("Qx=0x%0*x" % (int(div(nn, 4)), P[0]))
    print("Qy=0x%0*x" % (int(div(nn, 4)), P[1]))
    print("PplusQx=0x%0*x" % (int(div(nn, 4)), P[0]))
    print("PplusQy=0x%0*x" % (int(div(nn, 4)), P[1]))
    nbtest+=1
    #   Q = 0 (P != 0)
    print("== TEST P+Q #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: Q = 0, P /= 0")
    print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
    print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
    print("Q=0")
    print("PplusQx=0x%0*x" % (int(div(nn, 4)), P[0]))
    print("PplusQy=0x%0*x" % (int(div(nn, 4)), P[1]))
    nbtest+=1
    #   P = Q = 0
    print("== TEST P+Q #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: P = Q = 0")
    print("P=0")
    print("Q=0")
    print("PplusQ=0")
    nbtest+=1
    #   Q = [2]P and P is of order 3
    #   (this is actually already covered by P + Q test with P = -Q)
    #
    # EXCEPTIONS FOR PT_DBL ([2]P)
    #
    #   P = 0
    print("== TEST [2]P #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: P = 0")
    print("P=0")
    print("twoP=0")
    nbtest+=1
    #
    #   P of order 2 (aka 2-torsion)
    if nn <= NN_LIMIT_COMPUTE_Q:
        for fac in facs:
            if fac[0] == 2:
                # the order has 2 as a factor (possibly as a multifactor,
                # i.e with a height > 1)
                # compute the product of all other factors except this one
                # if it has a weight of 1, otherwise including it with an
                # exponent equal to its weight minus 1
                fs = 1
                for f in facs:
                    if f[0] != fac[0]:
                        fs = fs * (f[0]**f[1])
                # the line below is correct even if fac[1] == 1
                fs = fs * (2 ** (fac[1] - 1))
                # point fsP on line below is a point of order 2 (aka of 2-torsion)
                fsP = fs * P
                print("== TEST [2]P #%d.%d" % (nbcurv, nbtest))
                print("# EXCEPTION: P = 2-torsion")
                print("Px=0x%0*x" % (int(div(nn, 4)), fsP[0]))
                print("Py=0x%0*x" % (int(div(nn, 4)), fsP[1]))
                print("twoP=0")
                nbtest+=1
                # create a second test for exception of P + Q, w/ P = Q = 2-torsion
                print("== TEST P+Q #%d.%d" % (nbcurv, nbtest))
                print("# EXCEPTION: P = Q = 2-torsion")
                print("Px=0x%0*x" % (int(div(nn, 4)), fsP[0]))
                print("Py=0x%0*x" % (int(div(nn, 4)), fsP[1]))
                print("Qx=0x%0*x" % (int(div(nn, 4)), fsP[0]))
                print("Qy=0x%0*x" % (int(div(nn, 4)), fsP[1]))
                print("PplusQ=0")
                nbtest+=1
                # create a third test for exception of isP==-Q w/ P = Q = 2-torsion
                print("== TEST isP==-Q #%d.%d" % (nbcurv, nbtest))
                print("# EXCEPTION: P = Q = 2-torsion")
                print("Px=0x%0*x" % (int(div(nn, 4)), fsP[0]))
                print("Py=0x%0*x" % (int(div(nn, 4)), fsP[1]))
                print("Qx=0x%0*x" % (int(div(nn, 4)), fsP[0]))
                print("Qy=0x%0*x" % (int(div(nn, 4)), fsP[1]))
                print("true")
                nbtest+=1
    #
    # EXCEPTIONS FOR PT_EQU (P == Q)
    #
    #   P = Q
    #   (this is actually already tested above)
    #
    #   P != Q
    #   (this is actually already tested above)
    #
    #   P = -Q
    print("== TEST isP==Q #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: P = -Q")
    if P == 0:
        print("P=0")
    else:
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
    if P == 0:
        print("Q=0")
    else:
        print("Qx=0x%0*x" % (int(div(nn, 4)), (-P)[0]))
        print("Qy=0x%0*x" % (int(div(nn, 4)), (-P)[1]))
    if P == -P:
        print("true")
    else:
        print("false")
    nbtest+=1
    #   P = 0 (Q != 0)
    if P != 0:
        print("== TEST isP==Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P = 0, Q != 0")
        print("P=0")
        print("Qx=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Qy=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("false")
        nbtest+=1
    #   Q = 0 (P != 0)
    if P != 0:
        print("== TEST isP==Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P != 0, Q = 0")
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("Q=0")
        print("false")
        nbtest+=1
    #   P = Q = 0
    print("== TEST isP==Q #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: P = Q = 0")
    print("P=0")
    print("Q=0")
    print("true")
    nbtest+=1
    #
    # EXCEPTIONS FOR PT_OPP (P == -Q)
    #
    #   P = Q & P != -Q
    if P != 0 and P != -P:
        print("== TEST isP==-Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P = Q & P != -Q")
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("Qx=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Qy=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("false")
        nbtest+=1
    #   P = -Q  & P != Q
    if P != 0 and P != -P:
        print("== TEST isP==-Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P = -Q & P != Q")
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("Qx=0x%0*x" % (int(div(nn, 4)), (-P)[0]))
        print("Qy=0x%0*x" % (int(div(nn, 4)), (-P)[1]))
        print("true")
        nbtest+=1
    #   P = Q & P = -Q   (means 2-torsion point)
    #   (this is actually already tested above)
    #
    #   P = 0 (Q != 0)
    if P != 0:
        print("== TEST isP==-Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P = 0, Q != 0")
        print("P=0")
        print("Qx=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Qy=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("false")
        nbtest+=1
    #   Q = 0 (P != 0)
    if P != 0:
        print("== TEST isP==-Q #%d.%d" % (nbcurv, nbtest))
        print("# EXCEPTION: P != 0, Q = 0")
        print("Px=0x%0*x" % (int(div(nn, 4)), P[0]))
        print("Py=0x%0*x" % (int(div(nn, 4)), P[1]))
        print("Q=0")
        print("false")
        nbtest+=1
    #   P = Q = 0
    print("== TEST isP==-Q #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: P = Q = 0")
    print("P=0")
    print("Q=0")
    print("true")
    nbtest+=1
    #
    # EXCEPTIONS FOR PT_NEG (-P)
    #
    #   P = 0
    print("== TEST -P #%d.%d" % (nbcurv, nbtest))
    print("# EXCEPTION: P = 0")
    print("P=0")
    print("negP=0")
    nbtest+=1
    # increment the nb of generated curves for test
    nbcurv+=1
