//Copyright 2015 Ryan Wick

//This file is part of Bandage.

//Bandage is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//Bandage is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with Bandage.  If not, see <http://www.gnu.org/licenses/>.


#include <QtTest/QtTest>
#include <QAction>
#include <QDebug>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include "bandagetests.h"
#include "../ogdf/basic/Graph.h"
#include "../ogdf/basic/GraphAttributes.h"
#include "../graph/assemblygraph.h"
#include "../program/settings.h"
#include "../blast/blastsearch.h"
#include "../ui/mygraphicsview.h"
#include "../program/memory.h"
#include "../graph/debruijnnode.h"
#include "../graph/debruijnedge.h"
#include "../graph/graphicsitemnode.h"
#include "../graph/graphicsitemedge.h"
#include "../program/globals.h"
#include "../command_line/commoncommandlinefunctions.h"
#include "../program/gafparser.h"
#include "../program/graphlayoutio.h"
#include "../program/bedannotations.h"
#include "../program/tanglepathsearch.h"
#include "../ui/gafpathsdialog.h"
#include "../ui/mainwindow.h"
#include "../ui/mygraphicsscene.h"
#include "../ui/nodesequencewidget.h"
#include "../ui/selectednodespathswidget.h"




void BandageTests::loadFastg()
{
    createGlobals();
    bool fastgGraphLoaded = g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    //Check that the graph loaded properly.
    QCOMPARE(fastgGraphLoaded, true);

    //Check that the appropriate number of nodes/edges are present.
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 88);
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphEdges.size(), 118);

    //Check the length of a couple nodes.
    DeBruijnNode * node1 = g_assemblyGraph->m_deBruijnGraphNodes["1+"];
    DeBruijnNode * node28 = g_assemblyGraph->m_deBruijnGraphNodes["28-"];
    QCOMPARE(node1->getLength(), 6070);
    QCOMPARE(node28->getLength(), 79);
}


void BandageTests::loadLastGraph()
{
    createGlobals();
    bool lastGraphLoaded = g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.LastGraph");

    //Check that the graph loaded properly.
    QCOMPARE(lastGraphLoaded, true);

    //Check that the appropriate number of nodes/edges are present.
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 34);
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphEdges.size(), 32);

    //Check the length of a couple nodes.
    DeBruijnNode * node1 = g_assemblyGraph->m_deBruijnGraphNodes["1+"];
    DeBruijnNode * node14 = g_assemblyGraph->m_deBruijnGraphNodes["14-"];
    QCOMPARE(node1->getLength(), 2000);
    QCOMPARE(node14->getLength(), 60);
}



void BandageTests::loadTrinity()
{
    createGlobals();
    bool trinityLoaded = g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.Trinity.fasta");

    //Check that the graph loaded properly.
    QCOMPARE(trinityLoaded, true);

    //Check that the appropriate number of nodes/edges are present.
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 1170);
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphEdges.size(), 1056);

    //Check the length of a couple nodes.
    DeBruijnNode * node10241 = g_assemblyGraph->m_deBruijnGraphNodes["4|c4_10241+"];
    DeBruijnNode * node3901 = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3901-"];
    QCOMPARE(node10241->getLength(), 1186);
    QCOMPARE(node3901->getLength(), 1);
}


//LastGraph files have no overlap in the edges, so these tests look at paths
//where the connections are simple.
void BandageTests::pathFunctionsOnLastGraph()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.LastGraph");

    QString pathStringFailure;
    Path testPath1 = Path::makeFromString("(1996) 9+, 13+ (5)", false, &pathStringFailure);
    Path testPath2 = Path::makeFromString("(1996) 9+, 13+ (5)", false, &pathStringFailure);
    Path testPath3 = Path::makeFromString("(1996) 9+, 13+ (6)", false, &pathStringFailure);
    Path testPath4 = Path::makeFromString("9+, 13+, 14-", false, &pathStringFailure);

    DeBruijnNode * node4Minus = g_assemblyGraph->m_deBruijnGraphNodes["4-"];
    DeBruijnNode * node9Plus = g_assemblyGraph->m_deBruijnGraphNodes["9+"];
    DeBruijnNode * node13Plus = g_assemblyGraph->m_deBruijnGraphNodes["13+"];
    DeBruijnNode * node14Minus = g_assemblyGraph->m_deBruijnGraphNodes["14+"];
    DeBruijnNode * node7Plus = g_assemblyGraph->m_deBruijnGraphNodes["7+"];

    QCOMPARE(testPath1.getLength(), 10);
    QCOMPARE(testPath1.getPathSequence(), QByteArray("GACCTATAGA"));
    QCOMPARE(testPath1.isEmpty(), false);
    QCOMPARE(testPath1.isCircular(), false);
    QCOMPARE(testPath1 == testPath2, true);
    QCOMPARE(testPath1 == testPath3, false);
    QCOMPARE(testPath1.haveSameNodes(testPath3), true);
    QCOMPARE(testPath1.hasNodeSubset(testPath4), true);
    QCOMPARE(testPath4.hasNodeSubset(testPath1), false);
    QCOMPARE(testPath1.getString(true), QString("(1996) 9+, 13+ (5)"));
    QCOMPARE(testPath1.getString(false), QString("(1996)9+,13+(5)"));
    QCOMPARE(testPath4.getString(true), QString("9+, 13+, 14-"));
    QCOMPARE(testPath4.getString(false), QString("9+,13+,14-"));
    QCOMPARE(testPath1.containsEntireNode(node13Plus), false);
    QCOMPARE(testPath4.containsEntireNode(node13Plus), true);
    QCOMPARE(testPath4.isInMiddleOfPath(node13Plus), true);
    QCOMPARE(testPath4.isInMiddleOfPath(node14Minus), false);
    QCOMPARE(testPath4.isInMiddleOfPath(node9Plus), false);

    Path testPath4Extended;
    QCOMPARE(testPath4.canNodeFitOnEnd(node7Plus, &testPath4Extended), true);
    QCOMPARE(testPath4Extended.getString(true), QString("9+, 13+, 14-, 7+"));
    QCOMPARE(testPath4.canNodeFitAtStart(node4Minus, &testPath4Extended), true);
    QCOMPARE(testPath4Extended.getString(true), QString("4-, 9+, 13+, 14-"));
}



//FASTG files have overlaps in the edges, so these tests look at paths where
//the overlap has to be removed from the path sequence.
void BandageTests::pathFunctionsOnFastg()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    QString pathStringFailure;
    Path testPath1 = Path::makeFromString("(50234) 6+, 26+, 23+, 26+, 24+ (200)", false, &pathStringFailure);
    Path testPath2 = Path::makeFromString("26+, 23+", true, &pathStringFailure);
    QCOMPARE(testPath1.getLength(), 1764);
    QCOMPARE(testPath2.getLength(), 1387);
    QCOMPARE(testPath1.isCircular(), false);
    QCOMPARE(testPath2.isCircular(), true);
}


//This function tests paths on a GFA file which keeps its sequences in the GFA
//file.
void BandageTests::pathFunctionsOnGfaSequencesInGraph()
{
    createGlobals();
    bool gfaLoaded = g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa");
    QCOMPARE(gfaLoaded, true);

    //Check that the number of nodes/edges.
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 18);
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphEdges.size(), 24);

    //Check a couple short paths.
    QString pathStringFailure;

    Path testPath1 = Path::makeFromString("232+, 277+", false, &pathStringFailure);
    QByteArray testPath1Sequence = "CCTTATACGAAGCCCAGGTTAATCCTGGGCTTTTTGTTGAATCTGATCATTGGTAGCAAACAGATCAGGATTGGTAATTTTGATGTTTTCCTGACAACTCCTGCAAAGCATCAGCCCAGCAAAAAGTTGTACATGTTCCGTTGATTCACAGAAGGCACATGGCTTAGGAAAAGAGATGATATTGGCTGAATTGACTAATTCTGTTGACATAAGAAGTAACCTTGGATTGTACATATTTATTTTTAATAAATTTCAATACTTTATACCTAATTTAGCCACTAAAATTTGTACATTATTGTATAATCCATGTGTACATATTATTTTTTATAATTTTCATTCACTTAGACCCAAAATAGTATTTATTTTTGTACAACACCATGTACAGAACAATAATCATATAAATCAAATTTTTAGCATAAAAAAGTCCATTAATTTTGTACACAATTCTGAAACTTAAAAATCTAAACTTTCATCAATTTTTTTCATAATTTCAATAAATTAACCTTAATTTTAAGATATATTCTGAAATTTGGTTTGAAAGCTGTTTTTACATTATATTTCAATACTTTAAATCAAAAAATTGGATATTTTTTGAAAAACTCAATGAAAGTTTATTTTTTATTTAAAAACAACTAGTTATATTAGTTTTTATCCATTTTTTTGAAACAGTTTCTATATGATAAAAAAACCTATAAAAACCATATCTAGCAAAGGTTTGAGGGTTATCATCTTTAGATGCGTGGTGTGTGACAAAAAAATCCCGGCATGTGCCGGATTCTGGATTAGAAAATTGGCTAAAGTGACGTAGGACTGGTGCTTGGTTTTACATGGAAAAAAGTATTTATTTTCTGGTTTATAAAAACGTAAAAAGATCAGTTTTTGTTCATTCATCCAGGTTAAAAATTTCAACCTAAAACTTTAATTATGAAAAGCTTCACAGAAAGCATTCAAATGCGATTTAAGAGCCTTTATCTAAAAAACATAGATCTTATAGCGAAAAACAGAAAACAGCTCAAAAAACGCAAAAGAGAGTGAAGTAAAGAGATGTTTTGACTTTAGATAGCTGCATAGAGCGAGTGTCTACGAGCGAACTATCAAAATTTGCGTCTAGACTCTCTGAAAAACATTTTTTTGCCCTCTTTAGCCTAAGAAAGCTTAATTTTCATGCAGAAATTTGCTCCTGGACCGAGCGTAGCGAGAAAAAAAGCTCATGAGCGAAGCGAATTCCGAGTTGCTTTTGCTTTTTCTTAAAGTCACGCAAGTATTAACCAAAAAATTGCCCCGACGAACTGAGCGAAAGCGAAGTTCAATAGAGTTTGAGCGAAGCGAAAACCAAGGGC";
    Path testPath2 = Path::makeFromString("277-, 232-", false, &pathStringFailure);
    QByteArray testPath2Sequence = "GCCCTTGGTTTTCGCTTCGCTCAAACTCTATTGAACTTCGCTTTCGCTCAGTTCGTCGGGGCAATTTTTTGGTTAATACTTGCGTGACTTTAAGAAAAAGCAAAAGCAACTCGGAATTCGCTTCGCTCATGAGCTTTTTTTCTCGCTACGCTCGGTCCAGGAGCAAATTTCTGCATGAAAATTAAGCTTTCTTAGGCTAAAGAGGGCAAAAAAATGTTTTTCAGAGAGTCTAGACGCAAATTTTGATAGTTCGCTCGTAGACACTCGCTCTATGCAGCTATCTAAAGTCAAAACATCTCTTTACTTCACTCTCTTTTGCGTTTTTTGAGCTGTTTTCTGTTTTTCGCTATAAGATCTATGTTTTTTAGATAAAGGCTCTTAAATCGCATTTGAATGCTTTCTGTGAAGCTTTTCATAATTAAAGTTTTAGGTTGAAATTTTTAACCTGGATGAATGAACAAAAACTGATCTTTTTACGTTTTTATAAACCAGAAAATAAATACTTTTTTCCATGTAAAACCAAGCACCAGTCCTACGTCACTTTAGCCAATTTTCTAATCCAGAATCCGGCACATGCCGGGATTTTTTTGTCACACACCACGCATCTAAAGATGATAACCCTCAAACCTTTGCTAGATATGGTTTTTATAGGTTTTTTTATCATATAGAAACTGTTTCAAAAAAATGGATAAAAACTAATATAACTAGTTGTTTTTAAATAAAAAATAAACTTTCATTGAGTTTTTCAAAAAATATCCAATTTTTTGATTTAAAGTATTGAAATATAATGTAAAAACAGCTTTCAAACCAAATTTCAGAATATATCTTAAAATTAAGGTTAATTTATTGAAATTATGAAAAAAATTGATGAAAGTTTAGATTTTTAAGTTTCAGAATTGTGTACAAAATTAATGGACTTTTTTATGCTAAAAATTTGATTTATATGATTATTGTTCTGTACATGGTGTTGTACAAAAATAAATACTATTTTGGGTCTAAGTGAATGAAAATTATAAAAAATAATATGTACACATGGATTATACAATAATGTACAAATTTTAGTGGCTAAATTAGGTATAAAGTATTGAAATTTATTAAAAATAAATATGTACAATCCAAGGTTACTTCTTATGTCAACAGAATTAGTCAATTCAGCCAATATCATCTCTTTTCCTAAGCCATGTGCCTTCTGTGAATCAACGGAACATGTACAACTTTTTGCTGGGCTGATGCTTTGCAGGAGTTGTCAGGAAAACATCAAAATTACCAATCCTGATCTGTTTGCTACCAATGATCAGATTCAACAAAAAGCCCAGGATTAACCTGGGCTTCGTATAAGG";
    QCOMPARE(testPath1.getPathSequence(), testPath1Sequence);
    QCOMPARE(testPath2.getPathSequence(), testPath2Sequence);
}


//This function tests paths on a GFA file which keeps its sequences in a
//separate FASTA file.
void BandageTests::pathFunctionsOnGfaSequencesInFasta()
{
    createGlobals();
    bool gfaLoaded = g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids_separate_sequences.gfa");
    QCOMPARE(gfaLoaded, true);

    //Check that the number of nodes/edges.
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 18);
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphEdges.size(), 24);

    //Since a node sequence hasn't be required yet, the nodes should still have
    //just '*' for their sequence. But they should still report the correct
    //length.
    DeBruijnNode * node282Plus = g_assemblyGraph->m_deBruijnGraphNodes["282+"];
    DeBruijnNode * node282Minus = g_assemblyGraph->m_deBruijnGraphNodes["282-"];
    QCOMPARE(node282Plus->sequenceIsMissing(), true);
    QCOMPARE(node282Minus->sequenceIsMissing(), true);
    QCOMPARE(node282Plus->getLength(), 1819);
    QCOMPARE(node282Minus->getLength(), 1819);

    //Check a couple short paths.
    QString pathStringFailure;
    Path testPath1 = Path::makeFromString("232+, 277+", false, &pathStringFailure);
    QByteArray testPath1Sequence = "CCTTATACGAAGCCCAGGTTAATCCTGGGCTTTTTGTTGAATCTGATCATTGGTAGCAAACAGATCAGGATTGGTAATTTTGATGTTTTCCTGACAACTCCTGCAAAGCATCAGCCCAGCAAAAAGTTGTACATGTTCCGTTGATTCACAGAAGGCACATGGCTTAGGAAAAGAGATGATATTGGCTGAATTGACTAATTCTGTTGACATAAGAAGTAACCTTGGATTGTACATATTTATTTTTAATAAATTTCAATACTTTATACCTAATTTAGCCACTAAAATTTGTACATTATTGTATAATCCATGTGTACATATTATTTTTTATAATTTTCATTCACTTAGACCCAAAATAGTATTTATTTTTGTACAACACCATGTACAGAACAATAATCATATAAATCAAATTTTTAGCATAAAAAAGTCCATTAATTTTGTACACAATTCTGAAACTTAAAAATCTAAACTTTCATCAATTTTTTTCATAATTTCAATAAATTAACCTTAATTTTAAGATATATTCTGAAATTTGGTTTGAAAGCTGTTTTTACATTATATTTCAATACTTTAAATCAAAAAATTGGATATTTTTTGAAAAACTCAATGAAAGTTTATTTTTTATTTAAAAACAACTAGTTATATTAGTTTTTATCCATTTTTTTGAAACAGTTTCTATATGATAAAAAAACCTATAAAAACCATATCTAGCAAAGGTTTGAGGGTTATCATCTTTAGATGCGTGGTGTGTGACAAAAAAATCCCGGCATGTGCCGGATTCTGGATTAGAAAATTGGCTAAAGTGACGTAGGACTGGTGCTTGGTTTTACATGGAAAAAAGTATTTATTTTCTGGTTTATAAAAACGTAAAAAGATCAGTTTTTGTTCATTCATCCAGGTTAAAAATTTCAACCTAAAACTTTAATTATGAAAAGCTTCACAGAAAGCATTCAAATGCGATTTAAGAGCCTTTATCTAAAAAACATAGATCTTATAGCGAAAAACAGAAAACAGCTCAAAAAACGCAAAAGAGAGTGAAGTAAAGAGATGTTTTGACTTTAGATAGCTGCATAGAGCGAGTGTCTACGAGCGAACTATCAAAATTTGCGTCTAGACTCTCTGAAAAACATTTTTTTGCCCTCTTTAGCCTAAGAAAGCTTAATTTTCATGCAGAAATTTGCTCCTGGACCGAGCGTAGCGAGAAAAAAAGCTCATGAGCGAAGCGAATTCCGAGTTGCTTTTGCTTTTTCTTAAAGTCACGCAAGTATTAACCAAAAAATTGCCCCGACGAACTGAGCGAAAGCGAAGTTCAATAGAGTTTGAGCGAAGCGAAAACCAAGGGC";
    Path testPath2 = Path::makeFromString("277-, 232-", false, &pathStringFailure);
    QByteArray testPath2Sequence = "GCCCTTGGTTTTCGCTTCGCTCAAACTCTATTGAACTTCGCTTTCGCTCAGTTCGTCGGGGCAATTTTTTGGTTAATACTTGCGTGACTTTAAGAAAAAGCAAAAGCAACTCGGAATTCGCTTCGCTCATGAGCTTTTTTTCTCGCTACGCTCGGTCCAGGAGCAAATTTCTGCATGAAAATTAAGCTTTCTTAGGCTAAAGAGGGCAAAAAAATGTTTTTCAGAGAGTCTAGACGCAAATTTTGATAGTTCGCTCGTAGACACTCGCTCTATGCAGCTATCTAAAGTCAAAACATCTCTTTACTTCACTCTCTTTTGCGTTTTTTGAGCTGTTTTCTGTTTTTCGCTATAAGATCTATGTTTTTTAGATAAAGGCTCTTAAATCGCATTTGAATGCTTTCTGTGAAGCTTTTCATAATTAAAGTTTTAGGTTGAAATTTTTAACCTGGATGAATGAACAAAAACTGATCTTTTTACGTTTTTATAAACCAGAAAATAAATACTTTTTTCCATGTAAAACCAAGCACCAGTCCTACGTCACTTTAGCCAATTTTCTAATCCAGAATCCGGCACATGCCGGGATTTTTTTGTCACACACCACGCATCTAAAGATGATAACCCTCAAACCTTTGCTAGATATGGTTTTTATAGGTTTTTTTATCATATAGAAACTGTTTCAAAAAAATGGATAAAAACTAATATAACTAGTTGTTTTTAAATAAAAAATAAACTTTCATTGAGTTTTTCAAAAAATATCCAATTTTTTGATTTAAAGTATTGAAATATAATGTAAAAACAGCTTTCAAACCAAATTTCAGAATATATCTTAAAATTAAGGTTAATTTATTGAAATTATGAAAAAAATTGATGAAAGTTTAGATTTTTAAGTTTCAGAATTGTGTACAAAATTAATGGACTTTTTTATGCTAAAAATTTGATTTATATGATTATTGTTCTGTACATGGTGTTGTACAAAAATAAATACTATTTTGGGTCTAAGTGAATGAAAATTATAAAAAATAATATGTACACATGGATTATACAATAATGTACAAATTTTAGTGGCTAAATTAGGTATAAAGTATTGAAATTTATTAAAAATAAATATGTACAATCCAAGGTTACTTCTTATGTCAACAGAATTAGTCAATTCAGCCAATATCATCTCTTTTCCTAAGCCATGTGCCTTCTGTGAATCAACGGAACATGTACAACTTTTTGCTGGGCTGATGCTTTGCAGGAGTTGTCAGGAAAACATCAAAATTACCAATCCTGATCTGTTTGCTACCAATGATCAGATTCAACAAAAAGCCCAGGATTAACCTGGGCTTCGTATAAGG";

    //Check the paths sequences. Doing so will trigger the loading of node
    //sequences from the FASTA file.
    QCOMPARE(testPath1.getPathSequence(), testPath1Sequence);
    QCOMPARE(testPath2.getPathSequence(), testPath2Sequence);

    //Now that the paths have been accessed, the node sequences should be
    //loaded (even the ones not in the path).
    QCOMPARE(node282Plus->sequenceIsMissing(), false);
    QCOMPARE(node282Minus->sequenceIsMissing(), false);
    QCOMPARE(node282Plus->getLength(), 1819);
    QCOMPARE(node282Minus->getLength(), 1819);
}


void BandageTests::graphLocationFunctions()
{
    //First do some tests with a FASTG, where the overlap results in a simpler
    //sitations: all positions have a reverse complement position in the
    //reverse complement node.
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");
    DeBruijnNode * node12Plus = g_assemblyGraph->m_deBruijnGraphNodes["12+"];
    DeBruijnNode * node3Plus = g_assemblyGraph->m_deBruijnGraphNodes["3+"];

    GraphLocation location1(node12Plus, 1);
    GraphLocation revCompLocation1 = location1.reverseComplementLocation();

    QCOMPARE(location1.getBase(), 'C');
    QCOMPARE(revCompLocation1.getBase(), 'G');
    QCOMPARE(revCompLocation1.getPosition(), 394);

    GraphLocation location2 = GraphLocation::endOfNode(node3Plus);
    QCOMPARE(location2.getPosition(), 5869);

    location2.moveLocation(-1);
    QCOMPARE(location2.getPosition(), 5868);

    location2.moveLocation(2);
    QCOMPARE(location2.getNode()->getName(), QString("38-"));
    QCOMPARE(location2.getPosition(), 1);

    GraphLocation location3;
    QCOMPARE(location2.isNull(), false);
    QCOMPARE(location3.isNull(), true);

    //Now look at a LastGraph file which is more complex.  Because of the
    //offset, reverse complement positions can be in different nodes and may
    //not even exist.
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.LastGraph");
    int kmer = g_assemblyGraph->m_kmer;
    DeBruijnNode * node13Plus = g_assemblyGraph->m_deBruijnGraphNodes["13+"];
    DeBruijnNode * node8Minus = g_assemblyGraph->m_deBruijnGraphNodes["8-"];

    GraphLocation location4 = GraphLocation::startOfNode(node13Plus);
    QCOMPARE(location4.getBase(), 'A');
    QCOMPARE(location4.getPosition(), 1);

    GraphLocation revCompLocation4 = location4.reverseComplementLocation();
    QCOMPARE(revCompLocation4.getBase(), 'T');
    QCOMPARE(revCompLocation4.getNode()->getName(), QString("13-"));
    QCOMPARE(revCompLocation4.getPosition(), node13Plus->getLength() - kmer + 1);

    GraphLocation location5 = GraphLocation::endOfNode(node8Minus);
    GraphLocation location6 = location5;
    location6.moveLocation(-60);
    GraphLocation revCompLocation5 = location5.reverseComplementLocation();
    GraphLocation revCompLocation6 = location6.reverseComplementLocation();
    QCOMPARE(revCompLocation5.isNull(), true);
    QCOMPARE(revCompLocation6.isNull(), false);
    QCOMPARE(revCompLocation6.getNode()->getName(), QString("8+"));
    QCOMPARE(revCompLocation6.getPosition(), 1);
}



void BandageTests::loadCsvData()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    QString errormsg;
    QStringList columns;
    bool coloursLoaded = false;
    g_assemblyGraph->loadCSV(getTestDirectory() + "test.csv", &columns, &errormsg, &coloursLoaded);

    DeBruijnNode * node6Plus = g_assemblyGraph->m_deBruijnGraphNodes["6+"];
    DeBruijnNode * node6Minus = g_assemblyGraph->m_deBruijnGraphNodes["6-"];
    DeBruijnNode * node7Plus = g_assemblyGraph->m_deBruijnGraphNodes["7+"];
    DeBruijnNode * node4Plus = g_assemblyGraph->m_deBruijnGraphNodes["4+"];
    DeBruijnNode * node4Minus = g_assemblyGraph->m_deBruijnGraphNodes["4-"];
    DeBruijnNode * node3Plus = g_assemblyGraph->m_deBruijnGraphNodes["3+"];
    DeBruijnNode * node5Minus = g_assemblyGraph->m_deBruijnGraphNodes["5-"];
    DeBruijnNode * node8Plus = g_assemblyGraph->m_deBruijnGraphNodes["8+"];
    DeBruijnNode * node9Plus = g_assemblyGraph->m_deBruijnGraphNodes["9+"];

    QCOMPARE(columns.size(), 3);
    QCOMPARE(errormsg, QString("There were 2 unmatched entries in the CSV."));

    QCOMPARE(node6Plus->getCsvLine(0), QString("SIX_PLUS"));
    QCOMPARE(node6Plus->getCsvLine(1), QString("6plus"));
    QCOMPARE(node6Plus->getCsvLine(2), QString("plus6"));
    QCOMPARE(node9Plus->getCsvLine(3), QString(""));
    QCOMPARE(node9Plus->getCsvLine(25), QString(""));

    QCOMPARE(node6Minus->getCsvLine(0), QString("SIX_MINUS"));
    QCOMPARE(node6Minus->getCsvLine(1), QString("6minus"));
    QCOMPARE(node6Minus->getCsvLine(2), QString("minus6"));

    QCOMPARE(node7Plus->getCsvLine(0), QString("SEVEN_PLUS"));
    QCOMPARE(node7Plus->getCsvLine(1), QString("7plus"));
    QCOMPARE(node7Plus->getCsvLine(2), QString("plus7"));

    QCOMPARE(node4Plus->getCsvLine(0), QString("FOUR_PLUS"));
    QCOMPARE(node4Plus->getCsvLine(1), QString("4plus"));
    QCOMPARE(node4Plus->getCsvLine(2), QString("plus4"));

    QCOMPARE(node4Minus->getCsvLine(0), QString("FOUR_MINUS"));
    QCOMPARE(node4Minus->getCsvLine(1), QString("4minus"));
    QCOMPARE(node4Minus->getCsvLine(2), QString("minus4"));

    QCOMPARE(node3Plus->getCsvLine(0), QString("THREE_PLUS"));
    QCOMPARE(node3Plus->getCsvLine(1), QString("3plus"));
    QCOMPARE(node3Plus->getCsvLine(2), QString("plus3"));

    QCOMPARE(node5Minus->getCsvLine(0), QString("FIVE_MINUS"));
    QCOMPARE(node5Minus->getCsvLine(1), QString(""));
    QCOMPARE(node5Minus->getCsvLine(2), QString(""));

    QCOMPARE(node8Plus->getCsvLine(0), QString("EIGHT_PLUS"));
    QCOMPARE(node8Plus->getCsvLine(1), QString("8plus"));
    QCOMPARE(node8Plus->getCsvLine(2), QString("plus8"));

    QCOMPARE(node9Plus->getCsvLine(0), QString("NINE_PLUS"));
    QCOMPARE(node9Plus->getCsvLine(1), QString("9plus"));
    QCOMPARE(node9Plus->getCsvLine(2), QString("plus9"));
    QCOMPARE(node9Plus->getCsvLine(3), QString(""));
    QCOMPARE(node9Plus->getCsvLine(4), QString(""));
    QCOMPARE(node9Plus->getCsvLine(5), QString(""));
}


void BandageTests::loadCsvDataTrinity()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.Trinity.fasta");

    QString errormsg;
    QStringList columns;
    bool coloursLoaded = false;
    g_assemblyGraph->loadCSV(getTestDirectory() + "test.Trinity.csv", &columns, &errormsg, &coloursLoaded);

    DeBruijnNode * node3912Plus = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3912+"];
    DeBruijnNode * node3912Minus = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3912-"];
    DeBruijnNode * node3914Plus = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3914+"];
    DeBruijnNode * node3915Plus = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3915+"];
    DeBruijnNode * node3923Plus = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3923+"];
    DeBruijnNode * node3924Plus = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3924+"];
    DeBruijnNode * node3940Plus = g_assemblyGraph->m_deBruijnGraphNodes["19|c0_3940+"];

    QCOMPARE(columns.size(), 1);

    QCOMPARE(node3912Plus->getCsvLine(0), QString("3912PLUS"));
    QCOMPARE(node3912Minus->getCsvLine(0), QString("3912MINUS"));
    QCOMPARE(node3914Plus->getCsvLine(0), QString("3914PLUS"));
    QCOMPARE(node3915Plus->getCsvLine(0), QString("3915PLUS"));
    QCOMPARE(node3923Plus->getCsvLine(0), QString("3923PLUS"));
    QCOMPARE(node3924Plus->getCsvLine(0), QString("3924PLUS"));
    QCOMPARE(node3940Plus->getCsvLine(0), QString("3940PLUS"));
}

void BandageTests::loadCustomColours()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    QString errormsg;
    int unmatchedNodes = 0;
    bool success = g_assemblyGraph->loadCustomColours(getTestDirectory() + "custom_colours.txt",
                                                      &errormsg, &unmatchedNodes);

    QCOMPARE(success, true);
    QCOMPARE(unmatchedNodes, 1);
    QCOMPARE(errormsg, QString("There were 1 unmatched entries in the custom colour file."));
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes["6+"]->getCustomColour(), QColor("red"));
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes["6-"]->getCustomColour(), QColor("red"));
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes["4+"]->getCustomColour(), QColor("#00FF00"));
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes["4-"]->getCustomColour(), QColor("#00FF00"));
    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes["3+"]->hasCustomColour(), false);
}

void BandageTests::blastSearch()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");
    g_settings->blastQueryFilename = getTestDirectory() + "test_queries1.fasta";
    createBlastTempDirectory();

    g_blastSearch->doAutoBlastSearch();

    BlastQuery * exact = g_blastSearch->m_blastQueries.getQueryFromName("test_query_exact");
    BlastQuery * one_mismatch = g_blastSearch->m_blastQueries.getQueryFromName("test_query_one_mismatch");
    BlastQuery * one_insertion = g_blastSearch->m_blastQueries.getQueryFromName("test_query_one_insertion");
    BlastQuery * one_deletion = g_blastSearch->m_blastQueries.getQueryFromName("test_query_one_deletion");

    QCOMPARE(exact->getLength(), 100);
    QCOMPARE(one_mismatch->getLength(), 100);
    QCOMPARE(one_insertion->getLength(), 101);
    QCOMPARE(one_deletion->getLength(), 99);

    QSharedPointer<BlastHit> exactHit = exact->getHits().at(0);
    QSharedPointer<BlastHit> one_mismatchHit = one_mismatch->getHits().at(0);
    QSharedPointer<BlastHit> one_insertionHit = one_insertion->getHits().at(0);
    QSharedPointer<BlastHit> one_deletionHit = one_deletion->getHits().at(0);

    QCOMPARE(exactHit->m_numberMismatches, 0);
    QCOMPARE(exactHit->m_numberGapOpens, 0);
    QCOMPARE(one_mismatchHit->m_numberMismatches, 1);
    QCOMPARE(one_mismatchHit->m_numberGapOpens, 0);
    QCOMPARE(one_insertionHit->m_numberMismatches, 0);
    QCOMPARE(one_insertionHit->m_numberGapOpens, 1);
    QCOMPARE(one_deletionHit->m_numberMismatches, 0);
    QCOMPARE(one_deletionHit->m_numberGapOpens, 1);

    QCOMPARE(exactHit->m_percentIdentity < 100.0, false);
    QCOMPARE(one_mismatchHit->m_percentIdentity < 100.0, true);
    QCOMPARE(one_insertionHit->m_percentIdentity < 100.0, true);
    QCOMPARE(one_deletionHit->m_percentIdentity < 100.0, true);

    deleteBlastTempDirectory();
}



void BandageTests::blastSearchFilters()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");
    g_settings->blastQueryFilename = getTestDirectory() + "test_queries2.fasta";
    createBlastTempDirectory();

    //First do the search with no filters
    g_blastSearch->doAutoBlastSearch();
    int unfilteredHitCount = g_blastSearch->m_allHits.size();

    //Now filter by e-value.
    g_settings->blastEValueFilter.on = true;
    g_settings->blastEValueFilter = SciNot(1.0, -5);
    g_blastSearch->doAutoBlastSearch();
    QCOMPARE(g_blastSearch->m_allHits.size(), 14);
    QCOMPARE(g_blastSearch->m_allHits.size() < unfilteredHitCount, true);

    //Now add a bit score filter.
    g_settings->blastBitScoreFilter.on = true;
    g_settings->blastBitScoreFilter = 100.0;
    g_blastSearch->doAutoBlastSearch();
    QCOMPARE(g_blastSearch->m_allHits.size(), 9);

    //Now add an alignment length filter.
    g_settings->blastAlignmentLengthFilter.on = true;
    g_settings->blastAlignmentLengthFilter = 100;
    g_blastSearch->doAutoBlastSearch();
    QCOMPARE(g_blastSearch->m_allHits.size(), 8);

    //Now add an identity filter.
    g_settings->blastIdentityFilter.on = true;
    g_settings->blastIdentityFilter = 50.0;
    g_blastSearch->doAutoBlastSearch();
    QCOMPARE(g_blastSearch->m_allHits.size(), 7);

    //Now add a query coverage filter.
    g_settings->blastQueryCoverageFilter.on = true;
    g_settings->blastQueryCoverageFilter = 90.0;
    g_blastSearch->doAutoBlastSearch();
    QCOMPARE(g_blastSearch->m_allHits.size(), 5);

    deleteBlastTempDirectory();
}



void BandageTests::graphScope()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    QString errorTitle;
    QString errorMessage;
    int drawnNodes;
    std::vector<DeBruijnNode *> startingNodes;

    g_settings->graphScope = WHOLE_GRAPH;
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = false;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 44);

    g_settings->graphScope = WHOLE_GRAPH;
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = true;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 88);

    g_settings->graphScope = AROUND_NODE;
    g_settings->startingNodes = "1";
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = false;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 1);

    g_settings->graphScope = AROUND_NODE;
    g_settings->startingNodes = "1";
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = true;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 2);

    g_settings->graphScope = AROUND_NODE;
    g_settings->startingNodes = "1+";
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = true;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 1);

    g_settings->graphScope = AROUND_NODE;
    g_settings->startingNodes = "1";
    g_settings->nodeDistance = 1;
    g_settings->doubleMode = false;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 3);

    g_settings->graphScope = AROUND_NODE;
    g_settings->startingNodes = "1";
    g_settings->nodeDistance = 2;
    g_settings->doubleMode = false;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 10);

    g_settings->graphScope = DEPTH_RANGE;
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = false;
    g_settings->minDepthRange = 0.0;
    g_settings->maxDepthRange = 211.0;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 43);

    g_settings->graphScope = DEPTH_RANGE;
    g_settings->nodeDistance = 10;
    g_settings->doubleMode = false;
    g_settings->minDepthRange = 0.0;
    g_settings->maxDepthRange = 211.0;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 43);

    g_settings->graphScope = DEPTH_RANGE;
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = false;
    g_settings->minDepthRange = 211.0;
    g_settings->maxDepthRange = 1000.0;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 1);

    g_settings->graphScope = DEPTH_RANGE;
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = false;
    g_settings->minDepthRange = 40.0;
    g_settings->maxDepthRange = 211.0;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 42);

    createBlastTempDirectory();

    g_settings->blastQueryFilename = getTestDirectory() + "test_queries1.fasta";
    g_blastSearch->doAutoBlastSearch();

    g_settings->graphScope = AROUND_BLAST_HITS;
    g_settings->nodeDistance = 0;
    g_settings->doubleMode = false;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "all");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 1);

    g_settings->graphScope = AROUND_BLAST_HITS;
    g_settings->nodeDistance = 1;
    g_settings->doubleMode = false;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "all");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 3);

    g_settings->graphScope = AROUND_BLAST_HITS;
    g_settings->nodeDistance = 2;
    g_settings->doubleMode = false;
    startingNodes = g_assemblyGraph->getStartingNodes(&errorTitle, &errorMessage, g_settings->doubleMode, g_settings->startingNodes, "all");
    g_assemblyGraph->buildOgdfGraphFromNodesAndEdges(startingNodes, g_settings->nodeDistance);
    g_assemblyGraph->layoutGraph();
    drawnNodes = g_assemblyGraph->getDrawnNodeCount();
    QCOMPARE(drawnNodes, 9);

    deleteBlastTempDirectory();
}

void BandageTests::commandLineSettings()
{
    createGlobals();
    QStringList commandLineSettings;

    commandLineSettings = QString("--scope entire").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->graphScope, WHOLE_GRAPH);

    commandLineSettings = QString("--scope aroundnodes").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->graphScope, AROUND_NODE);

    commandLineSettings = QString("--scope aroundblast").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->graphScope, AROUND_BLAST_HITS);

    commandLineSettings = QString("--scope depthrange").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->graphScope, DEPTH_RANGE);

    commandLineSettings = QString("--nodes 5+").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->startingNodes, QString("5+"));

    commandLineSettings = QString("--nodes 1,2,3").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->startingNodes, QString("1,2,3"));

    QCOMPARE(g_settings->startingNodesExactMatch, true);
    commandLineSettings = QString("--partial").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->startingNodesExactMatch, false);

    commandLineSettings = QString("--distance 12").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->nodeDistance.val, 12);

    commandLineSettings = QString("--mindepth 1.2").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minDepthRange.val, 1.2);

    commandLineSettings = QString("--maxdepth 2.1").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxDepthRange.val, 2.1);

    QCOMPARE(g_settings->doubleMode, false);
    commandLineSettings = QString("--double").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->doubleMode, true);

    QCOMPARE(g_settings->nodeLengthMode, AUTO_NODE_LENGTH);
    commandLineSettings = QString("--nodelen 10000").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->nodeLengthMode, MANUAL_NODE_LENGTH);
    QCOMPARE(g_settings->manualNodeLengthPerMegabase.val, 10000.0);

    commandLineSettings = QString("--iter 1").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->graphLayoutQuality.val, 1);

    commandLineSettings = QString("--nodewidth 4.2").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->averageNodeWidth.val, 4.2);

    commandLineSettings = QString("--depwidth 0.222").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->depthEffectOnWidth.val, 0.222);

    commandLineSettings = QString("--deppower 0.72").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->depthPower.val, 0.72);

    QCOMPARE(g_settings->displayNodeNames, false);
    commandLineSettings = QString("--names").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->displayNodeNames, true);

    QCOMPARE(g_settings->displayNodeLengths, false);
    commandLineSettings = QString("--lengths").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->displayNodeLengths, true);

    QCOMPARE(g_settings->displayNodeDepth, false);
    commandLineSettings = QString("--depth").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->displayNodeDepth, true);

    QCOMPARE(g_settings->displayBlastHits, false);
    commandLineSettings = QString("--blasthits").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->displayBlastHits, true);

    commandLineSettings = QString("--fontsize 5").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->labelFont.pointSize(), 5);

    commandLineSettings = QString("--edgecol #00ff00").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->edgeColour.name(), QString("#00ff00"));

    commandLineSettings = QString("--edgewidth 5.5").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->edgeWidth.val, 5.5);

    commandLineSettings = QString("--outcol #ff0000").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->outlineColour.name(), QString("#ff0000"));

    commandLineSettings = QString("--outline 0.123").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->outlineThickness.val, 0.123);

    commandLineSettings = QString("--selcol tomato").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->selectionColour.name(), QString("#ff6347"));

    QCOMPARE(g_settings->antialiasing, true);
    commandLineSettings = QString("--noaa").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->antialiasing, false);

    commandLineSettings = QString("--textcol #550000ff").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->textColour.name(), QString("#0000ff"));
    QCOMPARE(g_settings->textColour.alpha(), 85);

    commandLineSettings = QString("--toutcol steelblue").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(getColourName(g_settings->textOutlineColour), QString("steelblue"));

    commandLineSettings = QString("--toutline 0.321").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->textOutlineThickness.val, 0.321);

    QCOMPARE(g_settings->positionTextNodeCentre, false);
    commandLineSettings = QString("--centre").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->positionTextNodeCentre, true);

    commandLineSettings = QString("--colour random").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->nodeColourScheme, RANDOM_COLOURS);

    commandLineSettings = QString("--colour uniform").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->nodeColourScheme, UNIFORM_COLOURS);

    commandLineSettings = QString("--colour depth").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->nodeColourScheme, DEPTH_COLOUR);

    commandLineSettings = QString("--colour blastsolid").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->nodeColourScheme, BLAST_HITS_SOLID_COLOUR);

    commandLineSettings = QString("--colour blastrainbow").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->nodeColourScheme, BLAST_HITS_RAINBOW_COLOUR);

    commandLineSettings = QString("--ransatpos 12").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->randomColourPositiveSaturation.val, 12);

    commandLineSettings = QString("--ransatneg 23").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->randomColourNegativeSaturation.val, 23);

    commandLineSettings = QString("--ranligpos 34").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->randomColourPositiveLightness.val, 34);

    commandLineSettings = QString("--ranligneg 45").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->randomColourNegativeLightness.val, 45);

    commandLineSettings = QString("--ranopapos 56").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->randomColourPositiveOpacity.val, 56);

    commandLineSettings = QString("--ranopaneg 67").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->randomColourNegativeOpacity.val, 67);

    commandLineSettings = QString("--unicolpos springgreen").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(getColourName(g_settings->uniformPositiveNodeColour), QString("springgreen"));

    commandLineSettings = QString("--unicolneg teal").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(getColourName(g_settings->uniformNegativeNodeColour), QString("teal"));

    commandLineSettings = QString("--unicolspe papayawhip").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(getColourName(g_settings->uniformNodeSpecialColour), QString("papayawhip"));

    commandLineSettings = QString("--depcollow mediumorchid").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(getColourName(g_settings->lowDepthColour), QString("mediumorchid"));

    commandLineSettings = QString("--depcolhi linen").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(getColourName(g_settings->highDepthColour), QString("linen"));

    QCOMPARE(g_settings->autoDepthValue, true);
    commandLineSettings = QString("--depvallow 56.7").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->lowDepthValue.val, 56.7);
    QCOMPARE(g_settings->autoDepthValue, false);

    commandLineSettings = QString("--depvalhi 67.8").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->highDepthValue.val, 67.8);

    commandLineSettings = QString("--depvalhi 67.8").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->highDepthValue.val, 67.8);

    commandLineSettings = QString("--query queries.fasta").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->blastQueryFilename, QString("queries.fasta"));

    commandLineSettings = QString("--blastp --abc").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->blastSearchParameters, QString("--abc"));

    QCOMPARE(g_settings->blastAlignmentLengthFilter.on, false);
    commandLineSettings = QString("--alfilter 543").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->blastAlignmentLengthFilter.on, true);
    QCOMPARE(g_settings->blastAlignmentLengthFilter.val, 543);

    QCOMPARE(g_settings->blastQueryCoverageFilter.on, false);
    commandLineSettings = QString("--qcfilter 67.8").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->blastQueryCoverageFilter.on, true);
    QCOMPARE(g_settings->blastQueryCoverageFilter.val, 67.8);

    QCOMPARE(g_settings->blastIdentityFilter.on, false);
    commandLineSettings = QString("--ifilter 12.3").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->blastIdentityFilter.on, true);
    QCOMPARE(g_settings->blastIdentityFilter.val, 12.3);

    QCOMPARE(g_settings->blastEValueFilter.on, false);
    commandLineSettings = QString("--evfilter 8.5e-14").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->blastEValueFilter.on, true);
    QCOMPARE(g_settings->blastEValueFilter.val, SciNot(8.5, -14));
    QCOMPARE(g_settings->blastEValueFilter.val.getCoefficient(), 8.5);
    QCOMPARE(g_settings->blastEValueFilter.val.getExponent(), -14);

    QCOMPARE(g_settings->blastBitScoreFilter.on, false);
    commandLineSettings = QString("--bsfilter 1234.5").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->blastBitScoreFilter.on, true);
    QCOMPARE(g_settings->blastBitScoreFilter.val, 1234.5);

    commandLineSettings = QString("--pathnodes 3").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxQueryPathNodes.val, 3);

    commandLineSettings = QString("--minpatcov 0.543").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minQueryCoveredByPath.val, 0.543);

    commandLineSettings = QString("--minhitcov 0.654").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minQueryCoveredByHits.val, 0.654);
    QCOMPARE(g_settings->minQueryCoveredByHits.on, true);

    commandLineSettings = QString("--minhitcov off").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minQueryCoveredByHits.on, false);

    commandLineSettings = QString("--minmeanid 0.765").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minMeanHitIdentity.val, 0.765);
    QCOMPARE(g_settings->minMeanHitIdentity.on, true);

    commandLineSettings = QString("--minmeanid off").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minMeanHitIdentity.on, false);

    commandLineSettings = QString("--minpatlen 0.97").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minLengthPercentage.val, 0.97);
    QCOMPARE(g_settings->minLengthPercentage.on, true);

    commandLineSettings = QString("--minpatlen off").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minLengthPercentage.on, false);

    commandLineSettings = QString("--maxpatlen 1.03").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxLengthPercentage.val, 1.03);
    QCOMPARE(g_settings->maxLengthPercentage.on, true);

    commandLineSettings = QString("--maxpatlen off").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxLengthPercentage.on, false);

    commandLineSettings = QString("--minlendis -1234").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minLengthBaseDiscrepancy.val, -1234);
    QCOMPARE(g_settings->minLengthBaseDiscrepancy.on, true);

    commandLineSettings = QString("--minlendis off").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->minLengthBaseDiscrepancy.on, false);

    commandLineSettings = QString("--maxlendis 4321").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxLengthBaseDiscrepancy.val, 4321);
    QCOMPARE(g_settings->maxLengthBaseDiscrepancy.on, true);

    commandLineSettings = QString("--maxlendis off").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxLengthBaseDiscrepancy.on, false);

    commandLineSettings = QString("--maxevprod 4e-500").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxEValueProduct.val.getCoefficient(), 4.0);
    QCOMPARE(g_settings->maxEValueProduct.val.getExponent(), -500);
    QCOMPARE(g_settings->maxEValueProduct.on, true);

    commandLineSettings = QString("--maxevprod off").split(" ");
    parseSettings(commandLineSettings);
    QCOMPARE(g_settings->maxEValueProduct.on, false);
}


void BandageTests::sciNotComparisons()
{
    SciNot sn01(1.0, 10);
    SciNot sn02(10.0, 9);
    SciNot sn03(0.1, 11);
    SciNot sn04(5.0, 10);
    SciNot sn05(-5.0, 15);
    SciNot sn06(-6.0, 15);
    SciNot sn07(-3.0, 3);
    SciNot sn08(-0.3, 4);
    SciNot sn09(-3.0, -3);
    SciNot sn10(-0.3, -2);
    SciNot sn11(1.4, 2);
    SciNot sn12("1.4e2");
    SciNot sn13("140");

    QCOMPARE(sn01 == sn02, true);
    QCOMPARE(sn01 == sn03, true);
    QCOMPARE(sn01 == sn03, true);
    QCOMPARE(sn01 >= sn03, true);
    QCOMPARE(sn01 <= sn03, true);
    QCOMPARE(sn01 != sn03, false);
    QCOMPARE(sn01 < sn04, true);
    QCOMPARE(sn01 <= sn04, true);
    QCOMPARE(sn01 > sn04, false);
    QCOMPARE(sn01 >= sn04, false);
    QCOMPARE(sn04 > sn05, true);
    QCOMPARE(sn04 < sn05, false);
    QCOMPARE(sn06 < sn05, true);
    QCOMPARE(sn06 <= sn05, true);
    QCOMPARE(sn06 > sn05, false);
    QCOMPARE(sn06 >= sn05, false);
    QCOMPARE(sn07 == sn08, true);
    QCOMPARE(sn07 != sn08, false);
    QCOMPARE(sn09 == sn10, true);
    QCOMPARE(sn09 != sn10, false);
    QCOMPARE(sn11 == sn12, true);
    QCOMPARE(sn11 == sn13, true);
    QCOMPARE(sn01.asString(true), QString("1e10"));
    QCOMPARE(sn01.asString(false), QString("1e10"));
    QCOMPARE(sn11.asString(true), QString("1.4e2"));
    QCOMPARE(sn11.asString(false), QString("140"));
}


void BandageTests::graphEdits()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 88);
    QCOMPARE(int(g_assemblyGraph->m_deBruijnGraphEdges.size()), 118);

    //Get the path sequence now to compare to the merged node sequence at the
    //end.
    QString pathStringFailure;
    Path path = Path::makeFromString("6+, 26+, 23+, 26+, 24+", false, &pathStringFailure);
    QByteArray pathSequence = path.getPathSequence();

    g_assemblyGraph->duplicateNodePair(g_assemblyGraph->m_deBruijnGraphNodes["26+"], 0);

    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 90);
    QCOMPARE(int(g_assemblyGraph->m_deBruijnGraphEdges.size()), 126);

    std::vector<DeBruijnEdge *> edgesToRemove;
    edgesToRemove.push_back(getEdgeFromNodeNames("26_copy+", "24+"));
    edgesToRemove.push_back(getEdgeFromNodeNames("6+", "26+"));
    edgesToRemove.push_back(getEdgeFromNodeNames("26+", "23+"));
    edgesToRemove.push_back(getEdgeFromNodeNames("23+", "26_copy+"));
    g_assemblyGraph->deleteEdges(&edgesToRemove);

    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 90);
    QCOMPARE(int(g_assemblyGraph->m_deBruijnGraphEdges.size()), 118);

    g_assemblyGraph->mergeAllPossible();

    QCOMPARE(g_assemblyGraph->m_deBruijnGraphNodes.size(), 82);
    QCOMPARE(int(g_assemblyGraph->m_deBruijnGraphEdges.size()), 110);

    DeBruijnNode * mergedNode = g_assemblyGraph->m_deBruijnGraphNodes["6_26_copy_23_26_24+"];
    QCOMPARE(pathSequence, mergedNode->getSequence());
}


void BandageTests::customLabelsDisplay()
{
    createGlobals();

    DeBruijnNode nodePlus("1+", 1.0, "ACGT");
    DeBruijnNode nodeMinus("1-", 1.0, "ACGT");
    nodePlus.setReverseComplement(&nodeMinus);
    nodeMinus.setReverseComplement(&nodePlus);

    nodePlus.setCustomLabel("plus-line-1\\nplus-line-2");
    nodeMinus.setCustomLabel("minus-line");

    g_settings->doubleMode = false;
    QCOMPARE(nodePlus.getCustomLabelForDisplay(), QStringList() << "plus-line-1" << "plus-line-2");
    QCOMPARE(nodeMinus.getCustomLabelForDisplay(), QStringList() << "minus-line");

    g_settings->doubleMode = true;
    QCOMPARE(nodePlus.getCustomLabelForDisplay(), QStringList() << "plus-line-1" << "plus-line-2");
    QCOMPARE(nodeMinus.getCustomLabelForDisplay(), QStringList() << "minus-line");
}


void BandageTests::gfaTagNodeLabels()
{
    createGlobals();
    MainWindow window;

    QTemporaryFile gfaFile(QDir::tempPath() + "/bandage-gfa-tags-XXXXXX.gfa");
    QVERIFY(gfaFile.open());
    QTextStream out(&gfaFile);
    out << "H\tVN:Z:1.0\n";
    out << "S\talpha\tAAAA\tXX:Z:first value\tSC:f:1.5\n";
    out << "S\tbeta\tCCCC\tYY:i:7\n";
    out.flush();
    gfaFile.close();

    QVERIFY(g_assemblyGraph->loadGraphFromFile(gfaFile.fileName()));
    DeBruijnNode * alpha = g_assemblyGraph->m_deBruijnGraphNodes.value("alpha+");
    QVERIFY(alpha != 0);
    QCOMPARE(alpha->getGfaExtraSegmentTags().size(), 2);
    window.setupGfaTagComboBox();
    window.setUiState(GRAPH_DRAWN);

    QCheckBox * tagCheckBox = window.findChild<QCheckBox *>("gfaTagCheckBox");
    QComboBox * tagComboBox = window.findChild<QComboBox *>("gfaTagComboBox");
    QVERIFY(tagCheckBox != 0);
    QVERIFY(tagComboBox != 0);
    QVERIFY(tagCheckBox->isEnabled());
    QVERIFY(tagComboBox->isEnabled());
    QCOMPARE(tagComboBox->count(), 3);
    QVERIFY(tagComboBox->findText("SC") >= 0);
    QVERIFY(tagComboBox->findText("XX") >= 0);
    QVERIFY(tagComboBox->findText("YY") >= 0);

    DeBruijnNode * alphaReverse = g_assemblyGraph->m_deBruijnGraphNodes["alpha-"];
    QVERIFY(alphaReverse != 0);
    QCOMPARE(alpha->getGfaTagValue("XX"), QString("first value"));
    QCOMPARE(alphaReverse->getGfaTagValue("XX"), QString("first value"));

    std::vector<QPointF> points;
    points.push_back(QPointF(0.0, 0.0));
    points.push_back(QPointF(50.0, 0.0));
    GraphicsItemNode alphaItem(alpha, points);

    tagComboBox->setCurrentText("XX");
    tagCheckBox->setChecked(false);
    window.setTextDisplaySettings();
    QVERIFY(alphaItem.getNodeText().isEmpty());

    tagCheckBox->setChecked(true);
    window.setTextDisplaySettings();
    QCOMPARE(alphaItem.getNodeText(), QStringList() << "first value");

    tagComboBox->setCurrentText("SC");
    window.setTextDisplaySettings();
    QCOMPARE(alphaItem.getNodeText(), QStringList() << "1.5");
}


void BandageTests::tangleBeamSearchAndGafClipping()
{
    const TanglePathParameters cpDefaults;
    QCOMPARE(cpDefaults.cpTauMin, 0.9);
    QCOMPARE(cpDefaults.fullThreadFraction, 0.4);
    QCOMPARE(cpDefaults.contextFraction, 0.6);
    QCOMPARE(cpDefaults.asFraction, 0.999);
    QCOMPARE(cpDefaults.coverageWeight, 0.75);
    QCOMPARE(cpDefaults.readWeight, 1.25);

    createGlobals();

    QTemporaryFile gfaFile(QDir::tempPath() + "/bandage-tangle-XXXXXX.gfa");
    QVERIFY(gfaFile.open());
    QTextStream gfa(&gfaFile);
    gfa << "H\tVN:Z:1.0\n";
    gfa << "S\tX\tAAAA\trd:i:10\n";
    gfa << "S\tA\tAAAA\trd:i:10\n";
    gfa << "S\tB\tAAAA\trd:i:10\n";
    gfa << "S\tC\tAAAA\trd:i:10\n";
    gfa << "S\tD\tAAAA\trd:i:10\n";
    gfa << "S\tY\tAAAA\trd:i:10\n";
    gfa << "L\tX\t+\tA\t+\t0M\n";
    gfa << "L\tA\t+\tB\t+\t0M\n";
    gfa << "L\tA\t+\tC\t+\t0M\n";
    gfa << "L\tB\t+\tD\t+\t0M\n";
    gfa << "L\tC\t+\tD\t+\t0M\n";
    gfa << "L\tD\t+\tY\t+\t0M\n";
    gfa.flush();
    gfaFile.close();
    QVERIFY(g_assemblyGraph->loadGraphFromFile(gfaFile.fileName()));

    std::vector<DeBruijnNode *> selectedNodes;
    selectedNodes.push_back(g_assemblyGraph->m_deBruijnGraphNodes.value("A+"));
    selectedNodes.push_back(g_assemblyGraph->m_deBruijnGraphNodes.value("B+"));
    selectedNodes.push_back(g_assemblyGraph->m_deBruijnGraphNodes.value("C+"));
    selectedNodes.push_back(g_assemblyGraph->m_deBruijnGraphNodes.value("D+"));
    QCOMPARE(commonNumericGfaTags(selectedNodes), QStringList() << "rd");

    TangleGraph graph;
    QStringList invalidCoverageNodes;
    QString graphError;
    QVERIFY(buildTangleGraph(selectedNodes, "rd", &graph,
                             &invalidCoverageNodes, &graphError));
    QVERIFY2(graphError.isEmpty(), qPrintable(graphError));
    QCOMPARE(graph.segments.size(), 4);

    TanglePathSearchRequest request;
    request.algorithm = TANGLE_PATH_BEAM_SEARCH;
    request.graph = graph;
    request.source = "A";
    request.target = "D";
    request.parameters.topK = 10;
    const TanglePathSearchResult result = runBeamTanglePathSearch(request);
    QVERIFY2(result.errorMessage.isEmpty(), qPrintable(result.errorMessage));

    QStringList paths;
    for (int i = 0; i < result.candidates.size(); ++i)
        paths << result.candidates[i].orientedNodeNames.join(",");
    QVERIFY(paths.contains("A+,B+,D+"));
    QVERIFY(paths.contains("A+,C+,D+"));

    QTemporaryFile gafFile(QDir::tempPath() + "/bandage-tangle-XXXXXX.gaf");
    QVERIFY(gafFile.open());
    QTextStream gaf(&gafFile);
    gaf << "read1\t1000\t0\t1000\t+\t>X>A>B>D>Y\t24\t0\t24\t990\t1000\t60\tAS:i:990\n";
    gaf.flush();
    gafFile.close();
    const GafParseResult parsed = parseGafFile(gafFile.fileName());
    QCOMPARE(parsed.alignments.size(), 1);
    QVERIFY(parsed.alignments.first().hasCpSatMetrics());

    int discardedAlignmentCount = 0;
    const QVector<TangleReadAlignment> clipped = extractTangleReadAlignments(
                parsed.alignments, graph, &discardedAlignmentCount);
    QCOMPARE(discardedAlignmentCount, 0);
    QCOMPARE(clipped.size(), 1);
    QStringList clippedPath;
    for (int i = 0; i < clipped.first().path.size(); ++i)
        clippedPath << graph.label(clipped.first().path[i]);
    QCOMPARE(clippedPath, QStringList() << "A+" << "B+" << "D+");

#ifdef BANDAGE_WITH_ORTOOLS
    request.algorithm = TANGLE_PATH_CP_SAT;
    request.readAlignments = clipped;
    request.parameters.timeLimitSeconds = 10.0;
    const TanglePathSearchResult cpResult = runCpSatTanglePathSearch(request);
    QVERIFY2(cpResult.errorMessage.isEmpty(), qPrintable(cpResult.errorMessage));
    QCOMPARE(cpResult.candidates.size(), 1);
    QCOMPARE(cpResult.candidates.first().orientedNodeNames,
             QStringList() << "A+" << "B+" << "D+");
#endif
}


//Saving a Velvet graph to GFA is a bit complex because the node sequence offset
//must be filled in.  This function tests aspects of that process.
void BandageTests::velvetToGfa()
{
    //First load the graph as a LastGraph and pull out some information and a
    //circular path sequence.
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "big_test.LastGraph");

    int lastGraphNodeCount = g_assemblyGraph->m_nodeCount;
    int lastGraphEdgeCount= g_assemblyGraph->m_edgeCount;
    long long lastGraphTotalLength = g_assemblyGraph->m_totalLength;
    long long lastGraphShortestContig = g_assemblyGraph->m_shortestContig;
    long long lastGraphLongestContig = g_assemblyGraph->m_longestContig;

    QString pathStringFailure;
    Path lastGraphTestPath1 = Path::makeFromString("3176-, 3176+, 4125+, 3178-, 3283+, 3180+, 3177-", true, &pathStringFailure);
    Path lastGraphTestPath2 = Path::makeFromString("3368+, 3369+, 3230+, 3231+, 3370-, 3143+, 3144+, 3145+, 3240+, 3241+, 3788-, 3787-, 3231+, 3252+, 3241-, 3240-, 3145-, 4164-, 4220+, 3368-, 3367+", true, &pathStringFailure);
    QByteArray lastGraphTestPath1Sequence = lastGraphTestPath1.getPathSequence();
    QByteArray lastGraphTestPath2Sequence = lastGraphTestPath2.getPathSequence();

    //Now save the graph as a GFA and reload it and grab the same information.
    g_assemblyGraph->saveEntireGraphToGfa(getTestDirectory() + "big_test_temp.gfa");
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "big_test_temp.gfa");

    int gfaNodeCount = g_assemblyGraph->m_nodeCount;
    int gfaEdgeCount= g_assemblyGraph->m_edgeCount;
    long long gfaTotalLength = g_assemblyGraph->m_totalLength;
    long long gfaShortestContig = g_assemblyGraph->m_shortestContig;
    long long gfaLongestContig = g_assemblyGraph->m_longestContig;

    Path gfaTestPath1 = Path::makeFromString("3176-, 3176+, 4125+, 3178-, 3283+, 3180+, 3177-", true, &pathStringFailure);
    Path gfaTestPath2 = Path::makeFromString("3368+, 3369+, 3230+, 3231+, 3370-, 3143+, 3144+, 3145+, 3240+, 3241+, 3788-, 3787-, 3231+, 3252+, 3241-, 3240-, 3145-, 4164-, 4220+, 3368-, 3367+", true, &pathStringFailure);
    QByteArray gfaTestPath1Sequence = gfaTestPath1.getPathSequence();
    QByteArray gfaTestPath2Sequence = gfaTestPath2.getPathSequence();

    //Now we compare the LastGraph info to the GFA info to make sure they are
    //the same (or appropriately different).  The k-mer size for this graph is
    //51, so we expect each node to get 50 base pairs longer.
    QCOMPARE(lastGraphNodeCount, gfaNodeCount);
    QCOMPARE(lastGraphEdgeCount, gfaEdgeCount);
    QCOMPARE(lastGraphTotalLength + 50 * lastGraphNodeCount, gfaTotalLength);
    QCOMPARE(lastGraphShortestContig + 50, gfaShortestContig);
    QCOMPARE(lastGraphLongestContig + 50, gfaLongestContig);
    QCOMPARE(lastGraphTestPath1Sequence, gfaTestPath1Sequence);
    QCOMPARE(lastGraphTestPath2Sequence, gfaTestPath2Sequence);

    //Finally, delete the gfa file.
    QFile::remove(getTestDirectory() + "big_test_temp.gfa");
}


void BandageTests::spadesToGfa()
{
    //First load the graph as a FASTG and pull out some information and a
    //path sequence.
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    int fastgNodeCount = g_assemblyGraph->m_nodeCount;
    int fastgEdgeCount= g_assemblyGraph->m_edgeCount;
    long long fastgTotalLength = g_assemblyGraph->m_totalLength;
    long long fastgShortestContig = g_assemblyGraph->m_shortestContig;
    long long fastgLongestContig = g_assemblyGraph->m_longestContig;

    QString pathStringFailure;
    Path fastgTestPath1 = Path::makeFromString("24+, 14+, 39-, 43-, 42-, 2-, 4+, 33+, 35-, 31-, 44-, 27+, 9-, 28-, 44+, 31+, 36+", false, &pathStringFailure);
    Path fastgTestPath2 = Path::makeFromString("16+, 7+, 13+, 37+, 41-, 40-, 42+, 43+, 39+, 14-, 22-, 20+, 7-, 25-, 32-, 38+, 3-", false, &pathStringFailure);
    QByteArray fastgTestPath1Sequence = fastgTestPath1.getPathSequence();
    QByteArray fastgTestPath2Sequence = fastgTestPath2.getPathSequence();

    //Now save the graph as a GFA and reload it and grab the same information.
    g_assemblyGraph->saveEntireGraphToGfa(getTestDirectory() + "test_temp.gfa");
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_temp.gfa");

    int gfaNodeCount = g_assemblyGraph->m_nodeCount;
    int gfaEdgeCount= g_assemblyGraph->m_edgeCount;
    long long gfaTotalLength = g_assemblyGraph->m_totalLength;
    long long gfaShortestContig = g_assemblyGraph->m_shortestContig;
    long long gfaLongestContig = g_assemblyGraph->m_longestContig;

    Path gfaTestPath1 = Path::makeFromString("24+, 14+, 39-, 43-, 42-, 2-, 4+, 33+, 35-, 31-, 44-, 27+, 9-, 28-, 44+, 31+, 36+", false, &pathStringFailure);
    Path gfaTestPath2 = Path::makeFromString("16+, 7+, 13+, 37+, 41-, 40-, 42+, 43+, 39+, 14-, 22-, 20+, 7-, 25-, 32-, 38+, 3-", false, &pathStringFailure);
    QByteArray gfaTestPath1Sequence = gfaTestPath1.getPathSequence();
    QByteArray gfaTestPath2Sequence = gfaTestPath2.getPathSequence();

    //Now we compare the LastGraph info to the GFA info to make sure they are
    //the same (or appropriately different).  The k-mer size for this graph is
    //51, so we expect each node to get 50 base pairs longer.
    QCOMPARE(fastgNodeCount, gfaNodeCount);
    QCOMPARE(fastgEdgeCount, gfaEdgeCount);
    QCOMPARE(fastgTotalLength, gfaTotalLength);
    QCOMPARE(fastgShortestContig, gfaShortestContig);
    QCOMPARE(fastgLongestContig, gfaLongestContig);
    QCOMPARE(fastgTestPath1Sequence, gfaTestPath1Sequence);
    QCOMPARE(fastgTestPath2Sequence, gfaTestPath2Sequence);

    //Finally, delete the gfa file.
    QFile::remove(getTestDirectory() + "test_temp.gfa");
}


void BandageTests::mergeNodesOnGfa()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa");

    DeBruijnNode * node6Plus = g_assemblyGraph->m_deBruijnGraphNodes["6+"];
    DeBruijnNode * node280Plus = g_assemblyGraph->m_deBruijnGraphNodes["280+"];
    DeBruijnNode * node232Minus = g_assemblyGraph->m_deBruijnGraphNodes["232-"];
    DeBruijnNode * node333Plus = g_assemblyGraph->m_deBruijnGraphNodes["333+"];
    DeBruijnNode * node289Plus = g_assemblyGraph->m_deBruijnGraphNodes["289+"];
    DeBruijnNode * node283Plus = g_assemblyGraph->m_deBruijnGraphNodes["283+"];
    DeBruijnNode * node277Plus = g_assemblyGraph->m_deBruijnGraphNodes["277+"];
    DeBruijnNode * node297Plus = g_assemblyGraph->m_deBruijnGraphNodes["297+"];
    DeBruijnNode * node282Plus = g_assemblyGraph->m_deBruijnGraphNodes["282+"];

    //Create a path before merging the nodes.
    QString pathStringFailure;
    Path testPath1 = Path::makeFromString("6+, 280+, 232-, 333+, 289+, 283+", true, &pathStringFailure);
    QByteArray path1Sequence = testPath1.getPathSequence();

    int path1Length = testPath1.getLength();
    int nodeTotalLength = node6Plus->getLength() + node280Plus->getLength() + node232Minus->getLength() +
            node333Plus->getLength() + node289Plus->getLength() + node283Plus->getLength();

    //The k-mer size in this test is 81, so the path should remove all of those overlaps.
    QCOMPARE(path1Length, nodeTotalLength - 6 * 81);

    //Now remove excess nodes.
    std::vector<DeBruijnNode*> nodesToDelete;
    nodesToDelete.push_back(node277Plus);
    nodesToDelete.push_back(node297Plus);
    nodesToDelete.push_back(node282Plus);
    g_assemblyGraph->deleteNodes(&nodesToDelete);

    //There should now be six nodes in the graph (plus complements).
    QCOMPARE(12, g_assemblyGraph->m_deBruijnGraphNodes.size());

    //After a merge, there should be only one node (and its complement).
    g_assemblyGraph->mergeAllPossible();
    QCOMPARE(2, g_assemblyGraph->m_deBruijnGraphNodes.size());

    //That last node should have a length of its six constituent nodes, minus
    //the overlaps.
    DeBruijnNode * lastNode = g_assemblyGraph->m_deBruijnGraphNodes.first();
    QCOMPARE(lastNode->getLength(), nodeTotalLength - 5 * 81);

    //If we make a circular path with this node, its length should be equal to
    //the length of the path made before.
    Path testPath2 = Path::makeFromString(lastNode->getName(), true, &pathStringFailure);
    QCOMPARE(path1Length, testPath2.getLength());

    //The sequence of this second path should also match the sequence of the
    //first path.
    QByteArray path2Sequence = testPath2.getPathSequence();
    QCOMPARE(doCircularSequencesMatch(path1Sequence, path2Sequence), true);
}



void BandageTests::changeNodeNames()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    DeBruijnNode * node6Plus = g_assemblyGraph->m_deBruijnGraphNodes["6+"];
    DeBruijnNode * node6Minus = g_assemblyGraph->m_deBruijnGraphNodes["6-"];
    int nodeCountBefore = g_assemblyGraph->m_deBruijnGraphNodes.size();

    g_assemblyGraph->changeNodeName("6", "12345");

    DeBruijnNode * node12345Plus = g_assemblyGraph->m_deBruijnGraphNodes["12345+"];
    DeBruijnNode * node12345Minus = g_assemblyGraph->m_deBruijnGraphNodes["12345-"];
    int nodeCountAfter = g_assemblyGraph->m_deBruijnGraphNodes.size();

    QCOMPARE(node6Plus, node12345Plus);
    QCOMPARE(node6Minus, node12345Minus);
    QCOMPARE(nodeCountBefore, nodeCountAfter);
}

void BandageTests::changeNodeDepths()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");

    DeBruijnNode * node6Plus = g_assemblyGraph->m_deBruijnGraphNodes["6+"];
    DeBruijnNode * node6Minus = g_assemblyGraph->m_deBruijnGraphNodes["6-"];
    DeBruijnNode * node7Plus = g_assemblyGraph->m_deBruijnGraphNodes["7+"];
    DeBruijnNode * node7Minus = g_assemblyGraph->m_deBruijnGraphNodes["7-"];

    std::vector<DeBruijnNode *> nodes;
    nodes.push_back(node6Plus);
    nodes.push_back(node7Plus);

    //Complementary pairs should have the same depth.
    QCOMPARE(node6Plus->getDepth(), node6Minus->getDepth());
    QCOMPARE(node7Plus->getDepth(), node7Minus->getDepth());

    g_assemblyGraph->changeNodeDepth(&nodes, 0.5);

    //Check to make sure the change worked.
    QCOMPARE(0.5, node6Plus->getDepth());
    QCOMPARE(0.5, node6Minus->getDepth());
    QCOMPARE(0.5, node7Plus->getDepth());
    QCOMPARE(0.5, node7Minus->getDepth());
}

void BandageTests::blastQueryPaths()
{
    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_query_paths.gfa");

    Settings defaultSettings;
    g_settings->blastQueryFilename = getTestDirectory() + "test_query_paths.fasta";
    defaultSettings.blastQueryFilename = getTestDirectory() + "test_query_paths.fasta";

    createBlastTempDirectory();

    //Now filter by e-value to get only strong hits and do the BLAST search.
    g_settings->blastEValueFilter.on = true;
    g_settings->blastEValueFilter = SciNot(1.0, -5);

    QList<BlastQueryPath> query1Paths;
    QList<BlastQueryPath> query2Paths;
    QList<BlastQueryPath> query3Paths;
    QList<BlastQueryPath> query4Paths;
    QList<BlastQueryPath> query5Paths;
    QList<BlastQueryPath> query6Paths;
    QList<BlastQueryPath> query7Paths;

    //With the default settings, queries 1 to 5 should each have one path and
    //queries 6 and 7 should have 0 (because of their large inserts).
    g_blastSearch->doAutoBlastSearch();
    query1Paths = g_blastSearch->m_blastQueries.m_queries[0]->getPaths();
    query2Paths = g_blastSearch->m_blastQueries.m_queries[1]->getPaths();
    query3Paths = g_blastSearch->m_blastQueries.m_queries[2]->getPaths();
    query4Paths = g_blastSearch->m_blastQueries.m_queries[3]->getPaths();
    query5Paths = g_blastSearch->m_blastQueries.m_queries[4]->getPaths();
    query6Paths = g_blastSearch->m_blastQueries.m_queries[5]->getPaths();
    query7Paths = g_blastSearch->m_blastQueries.m_queries[6]->getPaths();
    QCOMPARE(query1Paths.size(), 1);
    QCOMPARE(query2Paths.size(), 1);
    QCOMPARE(query3Paths.size(), 1);
    QCOMPARE(query4Paths.size(), 1);
    QCOMPARE(query5Paths.size(), 1);
    QCOMPARE(query6Paths.size(), 0);
    QCOMPARE(query7Paths.size(), 0);

    //query2 has a mean hit identity of 0.98.
    g_settings->minMeanHitIdentity.on = true;
    g_settings->minMeanHitIdentity = 0.979;
    g_blastSearch->doAutoBlastSearch();
    query2Paths = g_blastSearch->m_blastQueries.m_queries[1]->getPaths();
    QCOMPARE(query2Paths.size(), 1);
    g_settings->minMeanHitIdentity = 0.981;
    g_blastSearch->doAutoBlastSearch();
    query2Paths = g_blastSearch->m_blastQueries.m_queries[1]->getPaths();
    QCOMPARE(query2Paths.size(), 0);

    //Turning the filter off should make the path return.
    g_settings->minMeanHitIdentity.on = false;
    g_blastSearch->doAutoBlastSearch();
    query2Paths = g_blastSearch->m_blastQueries.m_queries[1]->getPaths();
    QCOMPARE(query2Paths.size(), 1);
    *g_settings = defaultSettings;

    //query3 has a length discrepancy of -20.
    g_settings->minLengthBaseDiscrepancy.on = true;
    g_settings->minLengthBaseDiscrepancy = -20;
    g_blastSearch->doAutoBlastSearch();
    query3Paths = g_blastSearch->m_blastQueries.m_queries[2]->getPaths();
    QCOMPARE(query3Paths.size(), 1);
    g_settings->minLengthBaseDiscrepancy = -19;
    g_blastSearch->doAutoBlastSearch();
    query3Paths = g_blastSearch->m_blastQueries.m_queries[2]->getPaths();
    QCOMPARE(query3Paths.size(), 0);

    //Turning the filter off should make the path return.
    g_settings->minLengthBaseDiscrepancy.on = false;
    g_blastSearch->doAutoBlastSearch();
    query3Paths = g_blastSearch->m_blastQueries.m_queries[2]->getPaths();
    QCOMPARE(query3Paths.size(), 1);
    *g_settings = defaultSettings;

    //query4 has a length discrepancy of +20.
    g_settings->maxLengthBaseDiscrepancy.on = true;
    g_settings->maxLengthBaseDiscrepancy = 20;
    g_blastSearch->doAutoBlastSearch();
    query4Paths = g_blastSearch->m_blastQueries.m_queries[3]->getPaths();
    QCOMPARE(query4Paths.size(), 1);
    g_settings->maxLengthBaseDiscrepancy = 19;
    g_blastSearch->doAutoBlastSearch();
    query4Paths = g_blastSearch->m_blastQueries.m_queries[3]->getPaths();
    QCOMPARE(query4Paths.size(), 0);

    //Turning the filter off should make the path return.
    g_settings->maxLengthBaseDiscrepancy.on = false;
    g_blastSearch->doAutoBlastSearch();
    query4Paths = g_blastSearch->m_blastQueries.m_queries[3]->getPaths();
    QCOMPARE(query4Paths.size(), 1);
    *g_settings = defaultSettings;

    //query5 has a path through 4 nodes.
    g_settings->maxQueryPathNodes = 4;
    g_blastSearch->doAutoBlastSearch();
    query5Paths = g_blastSearch->m_blastQueries.m_queries[4]->getPaths();
    QCOMPARE(query5Paths.size(), 1);
    g_settings->maxQueryPathNodes = 3;
    g_blastSearch->doAutoBlastSearch();
    query5Paths = g_blastSearch->m_blastQueries.m_queries[4]->getPaths();
    QCOMPARE(query5Paths.size(), 0);
    *g_settings = defaultSettings;

    //By turning off length restrictions, queries 6 and 7 should get path with
    //a large insert in the middle.
    g_settings->minLengthPercentage.on = false;
    g_settings->maxLengthPercentage.on = false;
    g_settings->minLengthBaseDiscrepancy.on = false;
    g_settings->maxLengthBaseDiscrepancy.on = false;
    g_blastSearch->doAutoBlastSearch();
    query6Paths = g_blastSearch->m_blastQueries.m_queries[5]->getPaths();
    query7Paths = g_blastSearch->m_blastQueries.m_queries[6]->getPaths();
    QCOMPARE(query6Paths.size(), 1);
    QCOMPARE(query7Paths.size(), 1);

    //Adjusting on the max length restriction can allow query 6 to get a path
    //and then query 7.
    g_settings->maxLengthBaseDiscrepancy.on = true;
    g_settings->maxLengthBaseDiscrepancy = 1999;
    g_blastSearch->doAutoBlastSearch();
    query6Paths = g_blastSearch->m_blastQueries.m_queries[5]->getPaths();
    query7Paths = g_blastSearch->m_blastQueries.m_queries[6]->getPaths();
    QCOMPARE(query6Paths.size(), 1);
    QCOMPARE(query7Paths.size(), 0);
    g_settings->maxLengthBaseDiscrepancy = 2000;
    g_blastSearch->doAutoBlastSearch();
    query6Paths = g_blastSearch->m_blastQueries.m_queries[5]->getPaths();
    query7Paths = g_blastSearch->m_blastQueries.m_queries[6]->getPaths();
    QCOMPARE(query6Paths.size(), 1);
    QCOMPARE(query7Paths.size(), 1);
}


void BandageTests::bandageInfo()
{
    int n50 = 0;
    int shortestNode = 0;
    int firstQuartile = 0;
    int median = 0;
    int thirdQuartile = 0;
    int longestNode = 0;
    int componentCount = 0;
    int largestComponentLength = 0;


    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.LastGraph");
    g_assemblyGraph->getNodeStats(&n50, &shortestNode, &firstQuartile, &median, &thirdQuartile, &longestNode);
    g_assemblyGraph->getGraphComponentCountAndLargestComponentSize(&componentCount, &largestComponentLength);
    QCOMPARE(17, g_assemblyGraph->m_nodeCount);
    QCOMPARE(16, g_assemblyGraph->m_edgeCount);
    QCOMPARE(29939, g_assemblyGraph->m_totalLength);
    QCOMPARE(10, g_assemblyGraph->getDeadEndCount());
    QCOMPARE(2000, n50);
    QCOMPARE(59, shortestNode);
    QCOMPARE(2000, longestNode);
    QCOMPARE(1, componentCount);
    QCOMPARE(29939, largestComponentLength);

    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg");
    g_assemblyGraph->getNodeStats(&n50, &shortestNode, &firstQuartile, &median, &thirdQuartile, &longestNode);
    g_assemblyGraph->getGraphComponentCountAndLargestComponentSize(&componentCount, &largestComponentLength);
    QCOMPARE(44, g_assemblyGraph->m_nodeCount);
    QCOMPARE(59, g_assemblyGraph->m_edgeCount);
    QCOMPARE(214441, g_assemblyGraph->m_totalLength);
    QCOMPARE(0, g_assemblyGraph->getDeadEndCount());
    QCOMPARE(35628, n50);
    QCOMPARE(78, shortestNode);
    QCOMPARE(52213, longestNode);
    QCOMPARE(1, componentCount);
    QCOMPARE(214441, largestComponentLength);

    createGlobals();
    g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.Trinity.fasta");
    g_assemblyGraph->getNodeStats(&n50, &shortestNode, &firstQuartile, &median, &thirdQuartile, &longestNode);
    g_assemblyGraph->getGraphComponentCountAndLargestComponentSize(&componentCount, &largestComponentLength);
    QCOMPARE(149, g_assemblyGraph->getDeadEndCount());
    QCOMPARE(66, componentCount);
    QCOMPARE(9398, largestComponentLength);
}


void BandageTests::gafParser()
{
    createGlobals();
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));

    QTemporaryFile gafFile;
    QVERIFY(gafFile.open());
    QTextStream out(&gafFile);
    out << "read-valid\t2000\t0\t2000\t+\t>232>277\t2000\t0\t2000\t1990\t2000\t60\n";
    out << "read-invalid\t100\t0\t100\t+\t>not-in-graph\t100\t0\t100\t100\t100\t20\n";
    out.flush();
    gafFile.flush();

    GafParseResult result = parseGafFile(gafFile.fileName());
    QCOMPARE(result.alignments.size(), 1);
    QCOMPARE(result.alignments.first().queryName, QString("read-valid"));
    QCOMPARE(result.alignments.first().bandagePathString, QString("232+, 277+"));
    QCOMPARE(result.alignments.first().mappingQuality, 60);
    QCOMPARE(result.warnings.size(), 1);
}


void BandageTests::gafFilteredExportIncludesAllPages()
{
    createGlobals();
    MainWindow window;
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));

    const QString firstLine =
            "read-first\t2000\t0\t2000\t+\t>232>277\t2000\t0\t2000\t1990\t2000\t60\ttp:A:P";
    const QString filteredOutLine =
            "read-filtered-out\t2000\t0\t2000\t+\t>232>277\t2000\t0\t2000\t1990\t2000\t10\ttp:A:S";
    const QString lastLine =
            "read-last\t2000\t0\t2000\t+\t>232>277\t2000\t0\t2000\t1990\t2000\t50\tcg:Z:2000M";

    QTemporaryFile gafFile;
    QVERIFY(gafFile.open());
    QTextStream gafOut(&gafFile);
    gafOut << firstLine << "\n" << filteredOutLine << "\n" << lastLine << "\n";
    gafOut.flush();
    gafFile.flush();

    GafParseResult result = parseGafFile(gafFile.fileName());
    QCOMPARE(result.alignments.size(), 3);
    QCOMPARE(result.alignments.first().rawLine, firstLine);

    GafPathsDialog dialog(0, "input.gaf", result);
    GafPathsModel * model = dialog.findChild<GafPathsModel *>();
    QVERIFY(model != 0);
    model->setPageSize(1);
    model->setVisibleRows(QList<int>() << 0 << 2);
    model->setCurrentPage(1);
    QCOMPARE(model->pageCount(), 2);
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->data(model->index(0, 1)).toString(), QString("read-last"));

    QTemporaryFile outputFile;
    QVERIFY(outputFile.open());
    QString outputFileName = outputFile.fileName();
    outputFile.close();

    QString errorMessage;
    QVERIFY2(dialog.writeFilteredGaf(outputFileName, &errorMessage), qPrintable(errorMessage));

    QFile savedFile(outputFileName);
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream savedIn(&savedFile);
    QCOMPARE(savedIn.readAll(), firstLine + "\n" + lastLine + "\n");
}


void BandageTests::gafNodeFilterContainedMode()
{
    createGlobals();
    MainWindow window;
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));

    QTemporaryFile gafFile;
    QVERIFY(gafFile.open());
    QTextStream out(&gafFile);
    out << "read-single\t100\t0\t100\t+\t>232\t100\t0\t100\t100\t100\t60\n";
    out << "read-all\t200\t0\t200\t+\t>232>277\t200\t0\t200\t200\t200\t60\n";
    out << "read-extra\t200\t0\t200\t+\t>280<232\t200\t0\t200\t200\t200\t60\n";
    out << "read-none\t200\t0\t200\t+\t>297>289\t200\t0\t200\t200\t200\t60\n";
    out.flush();
    gafFile.flush();

    GafParseResult result = parseGafFile(gafFile.fileName());
    QCOMPARE(result.alignments.size(), 4);

    GafPathsDialog dialog(0, "input.gaf", result);
    GafPathsModel * model = dialog.findChild<GafPathsModel *>();
    QComboBox * modeComboBox = dialog.findChild<QComboBox *>();
    QVERIFY(model != 0);
    QVERIFY(modeComboBox != 0);

    QLineEdit * nodeFilterLineEdit = 0;
    const QList<QLineEdit *> lineEdits = dialog.findChildren<QLineEdit *>();
    for (int i = 0; i < lineEdits.size(); ++i)
    {
        if (lineEdits[i]->placeholderText() == "Node name(s)")
        {
            nodeFilterLineEdit = lineEdits[i];
            break;
        }
    }
    QVERIFY(nodeFilterLineEdit != 0);
    nodeFilterLineEdit->setText("232, 277");

    modeComboBox->setCurrentIndex(0);
    QVERIFY(QMetaObject::invokeMethod(&dialog, "filterByMapq", Qt::DirectConnection));
    QCOMPARE(model->totalRows(), 3);

    modeComboBox->setCurrentIndex(1);
    QVERIFY(QMetaObject::invokeMethod(&dialog, "filterByMapq", Qt::DirectConnection));
    QCOMPARE(model->totalRows(), 1);
    QCOMPARE(model->data(model->index(0, 1)).toString(), QString("read-all"));

    modeComboBox->setCurrentIndex(2);
    QVERIFY(QMetaObject::invokeMethod(&dialog, "filterByMapq", Qt::DirectConnection));
    QCOMPARE(model->totalRows(), 2);
    QCOMPARE(model->visibleRows(), QList<int>() << 0 << 1);
}


void BandageTests::gafHighlightUsesGraphSelection()
{
    createGlobals();
    MainWindow window;
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));

    QTemporaryFile gafFile;
    QVERIFY(gafFile.open());
    QTextStream out(&gafFile);
    out << "read-one\t2000\t0\t2000\t+\t>232>277\t2000\t0\t2000\t1990\t2000\t60\n";
    out.flush();
    gafFile.flush();

    QCOMPARE(window.loadGafPathsFile(gafFile.fileName(), MainWindow::FORCE_GAF_REPLACEMENT),
             MainWindow::GAF_LOADED);

    QList<Path> paths;
    paths << parseGafFile(gafFile.fileName()).alignments.first().path;
    g_memory->setQueryPaths(paths, Memory::GAF_PATH_HIGHLIGHT);
    QVERIFY(g_memory->queryPathHighlightIsVisible());

    QList<GraphicsItemNode *> pathItems;
    QList<DeBruijnNode *> pathNodes = paths.first().getNodes();
    for (int i = 0; i < pathNodes.size(); ++i)
    {
        std::vector<QPointF> points;
        points.push_back(QPointF(i * 100.0, 0.0));
        points.push_back(QPointF(i * 100.0 + 50.0, 0.0));
        GraphicsItemNode * item = new GraphicsItemNode(pathNodes[i], points);
        item->setFlag(QGraphicsItem::ItemIsSelectable);
        pathNodes[i]->setGraphicsItemNode(item);
        g_graphicsView->scene()->addItem(item);
        pathItems << item;
    }

    QGraphicsRectItem * previousSelection = g_graphicsView->scene()->addRect(-100.0, -100.0,
                                                                              10.0, 10.0);
    previousSelection->setFlag(QGraphicsItem::ItemIsSelectable);
    previousSelection->setSelected(true);

    GafPathsTableView * table = window.m_gafPathsWidget->findChild<GafPathsTableView *>();
    QVERIFY(table != 0);
    table->selectRow(0);
    QVERIFY(QMetaObject::invokeMethod(window.m_gafPathsWidget, "highlightSelectedPaths",
                                      Qt::DirectConnection));

    QCOMPARE(g_memory->pathHighlightSource, Memory::NO_PATH_HIGHLIGHT);
    QVERIFY(g_memory->queryPaths.isEmpty());
    QVERIFY(!previousSelection->isSelected());
    for (int i = 0; i < pathItems.size(); ++i)
        QVERIFY(pathItems[i]->isSelected());

    window.clearGafHighlighting();
    QVERIFY(g_graphicsView->scene()->selectedItems().isEmpty());
}


void BandageTests::selectedNodesPathHighlightUsesGraphSelection()
{
    createGlobals();
    MainWindow window;
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));

    QString failure;
    Path path = Path::makeFromString("232+, 277+", false, &failure);
    QVERIFY2(!path.isEmpty(), qPrintable(failure));
    QList<Path> paths;
    paths << path;

    window.showSelectedNodesPathsTab(paths);
    g_memory->setQueryPaths(paths, Memory::SELECTED_NODE_PATH_HIGHLIGHT);
    QVERIFY(g_memory->queryPathHighlightIsVisible());

    QList<GraphicsItemNode *> pathItems;
    QList<DeBruijnNode *> pathNodes = path.getNodes();
    for (int i = 0; i < pathNodes.size(); ++i)
    {
        std::vector<QPointF> points;
        points.push_back(QPointF(i * 100.0, 0.0));
        points.push_back(QPointF(i * 100.0 + 50.0, 0.0));
        GraphicsItemNode * item = new GraphicsItemNode(pathNodes[i], points);
        item->setFlag(QGraphicsItem::ItemIsSelectable);
        pathNodes[i]->setGraphicsItemNode(item);
        g_graphicsView->scene()->addItem(item);
        pathItems << item;
    }

    QGraphicsRectItem * previousSelection = g_graphicsView->scene()->addRect(-100.0, -100.0,
                                                                              10.0, 10.0);
    previousSelection->setFlag(QGraphicsItem::ItemIsSelectable);
    previousSelection->setSelected(true);

    QTableWidget * table = window.m_selectedNodesPathsWidget->findChild<QTableWidget *>();
    QVERIFY(table != 0);
    table->selectRow(0);
    QVERIFY(QMetaObject::invokeMethod(window.m_selectedNodesPathsWidget, "highlightSelectedPaths",
                                      Qt::DirectConnection));

    QCOMPARE(g_memory->pathHighlightSource, Memory::NO_PATH_HIGHLIGHT);
    QVERIFY(g_memory->queryPaths.isEmpty());
    QVERIFY(!previousSelection->isSelected());
    for (int i = 0; i < pathItems.size(); ++i)
        QVERIFY(pathItems[i]->isSelected());
}


void BandageTests::pathHighlightOwnership()
{
    createGlobals();
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));

    QString failure;
    Path path = Path::makeFromString("232+, 277+", false, &failure);
    QVERIFY2(!path.isEmpty(), qPrintable(failure));
    QList<Path> paths;
    paths << path;

    g_memory->setQueryPaths(paths, Memory::GAF_PATH_HIGHLIGHT);
    g_memory->gafPathDialogIsVisible = false;
    QVERIFY(g_memory->queryPathHighlightIsVisible());
    QVERIFY(!g_memory->clearQueryPaths(Memory::BLAST_QUERY_PATH_HIGHLIGHT));
    QCOMPARE(g_memory->pathHighlightSource, Memory::GAF_PATH_HIGHLIGHT);
    QCOMPARE(g_memory->queryPaths.size(), 1);

    g_memory->setQueryPaths(paths, Memory::SELECTED_NODE_PATH_HIGHLIGHT);
    g_memory->selectedPathsDialogIsVisible = false;
    QVERIFY(g_memory->queryPathHighlightIsVisible());
    QVERIFY(!g_memory->clearQueryPaths(Memory::GAF_PATH_HIGHLIGHT));

    g_memory->setQueryPaths(paths, Memory::BLAST_QUERY_PATH_HIGHLIGHT);
    g_memory->queryPathDialogIsVisible = false;
    QVERIFY(!g_memory->queryPathHighlightIsVisible());
    g_memory->queryPathDialogIsVisible = true;
    QVERIFY(g_memory->queryPathHighlightIsVisible());
    QVERIFY(g_memory->clearQueryPaths(Memory::BLAST_QUERY_PATH_HIGHLIGHT));
    QCOMPARE(g_memory->pathHighlightSource, Memory::NO_PATH_HIGHLIGHT);
    QVERIFY(g_memory->queryPaths.isEmpty());
}


void BandageTests::dynamicTabAndGafReplacement()
{
    createGlobals();
    MainWindow window;
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));

    QTemporaryFile firstGaf;
    QTemporaryFile secondGaf;
    QTemporaryFile invalidGaf;
    QVERIFY(firstGaf.open());
    QVERIFY(secondGaf.open());
    QVERIFY(invalidGaf.open());

    QTextStream firstOut(&firstGaf);
    firstOut << "read-one\t2000\t0\t2000\t+\t>232>277\t2000\t0\t2000\t1990\t2000\t60\n";
    firstOut.flush();
    firstGaf.flush();

    QTextStream secondOut(&secondGaf);
    secondOut << "read-two\t1800\t0\t1800\t+\t>277\t1800\t0\t1800\t1790\t1800\t50\n";
    secondOut.flush();
    secondGaf.flush();

    QTextStream invalidOut(&invalidGaf);
    invalidOut << "not-a-gaf-record\n";
    invalidOut.flush();
    invalidGaf.flush();

    window.showNodeSequenceTab(g_assemblyGraph->m_deBruijnGraphNodes["232+"]);
    QCOMPARE(window.loadGafPathsFile(firstGaf.fileName(), MainWindow::FORCE_GAF_REPLACEMENT),
             MainWindow::GAF_LOADED);
    QCOMPARE(window.m_tabWidget->count(), 3);
    QVERIFY(window.m_tabWidget->indexOf(window.m_nodeSequenceWidget) >= 0);
    QVERIFY(window.m_tabWidget->indexOf(window.m_gafPathsWidget) >= 0);
    QCOMPARE(window.findChildren<GafPathsDialog *>().size(), 1);

    GafPathsDialog * firstWidget = window.m_gafPathsWidget;
    QList<Path> highlightedPaths;
    highlightedPaths << parseGafFile(firstGaf.fileName()).alignments.first().path;
    g_memory->setQueryPaths(highlightedPaths, Memory::GAF_PATH_HIGHLIGHT);
    g_memory->gafPathDialogIsVisible = true;
    window.m_tabWidget->setCurrentIndex(0);
    QCoreApplication::processEvents();
    QCOMPARE(g_memory->pathHighlightSource, Memory::GAF_PATH_HIGHLIGHT);
    QVERIFY(g_memory->queryPathHighlightIsVisible());

    QTimer::singleShot(0, []()
    {
        QMessageBox * messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (messageBox != 0)
            messageBox->done(QMessageBox::No);
    });
    QCOMPARE(window.loadGafPathsFile(secondGaf.fileName(), MainWindow::CONFIRM_GAF_REPLACEMENT),
             MainWindow::GAF_LOAD_CANCELLED);
    QCOMPARE(window.m_gafPathsWidget, firstWidget);
    QCOMPARE(g_memory->pathHighlightSource, Memory::GAF_PATH_HIGHLIGHT);

    QTimer::singleShot(0, []()
    {
        QMessageBox * messageBox = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (messageBox != 0)
            messageBox->done(QMessageBox::Ok);
    });
    QCOMPARE(window.loadGafPathsFile(invalidGaf.fileName(), MainWindow::FORCE_GAF_REPLACEMENT),
             MainWindow::GAF_LOAD_FAILED);
    QCOMPARE(window.m_gafPathsWidget, firstWidget);
    QCOMPARE(g_memory->pathHighlightSource, Memory::GAF_PATH_HIGHLIGHT);
    QCOMPARE(window.findChildren<GafPathsDialog *>().size(), 1);

    window.showNodeSequenceTab(g_assemblyGraph->m_deBruijnGraphNodes["277+"]);
    QCOMPARE(window.loadGafPathsFile(secondGaf.fileName(), MainWindow::FORCE_GAF_REPLACEMENT),
             MainWindow::GAF_LOADED);
    QCOMPARE(window.m_tabWidget->count(), 3);
    QVERIFY(window.m_gafPathsWidget != firstWidget);
    QVERIFY(window.m_tabWidget->indexOf(window.m_nodeSequenceWidget) >= 0);
    QVERIFY(window.m_tabWidget->indexOf(window.m_gafPathsWidget) >= 0);
    QCOMPARE(window.findChildren<GafPathsDialog *>().size(), 1);
    QCOMPARE(window.m_gafPathsWidget->alignmentCount(), 1);
    QCOMPARE(g_memory->pathHighlightSource, Memory::NO_PATH_HIGHLIGHT);
    QVERIFY(g_memory->queryPaths.isEmpty());

    window.clearGafHighlighting();
    QCOMPARE(g_memory->pathHighlightSource, Memory::NO_PATH_HIGHLIGHT);
}
















void BandageTests::createGlobals()
{
    g_settings.reset(new Settings());
    g_memory.reset(new Memory());
    g_blastSearch.reset(new BlastSearch());
    g_assemblyGraph.reset(new AssemblyGraph());
    g_graphicsView = new MyGraphicsView();
}

bool BandageTests::createBlastTempDirectory()
{
    //Running from the command line, it makes more sense to put the temp
    //directory in the current directory.
    g_blastSearch->m_tempDirectory = "bandage_temp-" + QString::number(QCoreApplication::applicationPid()) + "/";

    if (!QDir().mkdir(g_blastSearch->m_tempDirectory))
        return false;

    g_blastSearch->m_blastQueries.createTempQueryFiles();
    return true;
}

void BandageTests::deleteBlastTempDirectory()
{
    if (g_blastSearch->m_tempDirectory != "" &&
            QDir(g_blastSearch->m_tempDirectory).exists() &&
            QDir(g_blastSearch->m_tempDirectory).dirName().contains("bandage_temp"))
        QDir(g_blastSearch->m_tempDirectory).removeRecursively();
}

QString BandageTests::getTestDirectory()
{
    QDir directory = QDir::current();

    //We want to find a directory "Bandage/tests/".  Keep backing up in the
    //directory structure until we find it.
    QString path;
    while (true)
    {
        path = directory.path() + "/Bandage/tests/";
        if (QDir(path).exists())
            return path;
        if (!directory.cdUp())
            return "";
    }

    return "";
}

DeBruijnEdge * BandageTests::getEdgeFromNodeNames(QString startingNodeName,
                                                  QString endingNodeName)
{
    DeBruijnNode * startingNode = g_assemblyGraph->m_deBruijnGraphNodes[startingNodeName];
    DeBruijnNode * endingNode = g_assemblyGraph->m_deBruijnGraphNodes[endingNodeName];

    QPair<DeBruijnNode*, DeBruijnNode*> nodePair(startingNode, endingNode);

    if (g_assemblyGraph->m_deBruijnGraphEdges.contains(nodePair))
        return g_assemblyGraph->m_deBruijnGraphEdges[nodePair];
    else
        return 0;
}


//This function checks to see if two circular sequences match.  It needs to
//check each possible rotation, as well as reverse complements.
bool BandageTests::doCircularSequencesMatch(QByteArray s1, QByteArray s2)
{
    for (int i = 0; i < s1.length() - 1; ++i)
    {
        QByteArray rotatedS1 = s1.right(s1.length() - i) + s1.left(i);
        if (rotatedS1 == s2)
            return true;
    }

    //If the code got here, then all possible rotations of s1 failed to match
    //s2.  Now we try the reverse complement.
    QByteArray s1Rc = AssemblyGraph::getReverseComplement(s1);
    for (int i = 0; i < s1Rc.length() - 1; ++i)
    {
        QByteArray rotatedS1Rc = s1Rc.right(s1Rc.length() - i) + s1Rc.left(i);
        if (rotatedS1Rc == s2)
            return true;
    }

    return false;
}



void BandageTests::graphLayoutImportExport()
{
    createGlobals();
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test.fastg"));

    SavedGraphLayout original;
    original.doubleMode = false;
    original.nodePoints.insert("1+", std::vector<QPointF>{QPointF(1.25, -3.5),
                                                          QPointF(9.0, 12.75)});
    original.nodePoints.insert("12+", std::vector<QPointF>{QPointF(-4.0, 8.0),
                                                           QPointF(5.5, 6.25),
                                                           QPointF(13.0, 7.0)});

    QTemporaryDir layoutDirectory(QDir::tempPath() + "/bandage-layout-XXXXXX");
    QVERIFY(layoutDirectory.isValid());
    const QString layoutFilename = layoutDirectory.filePath("graph.layout");

    QString error;
    QVERIFY2(saveGraphLayout(layoutFilename, *g_assemblyGraph, original, &error),
             qPrintable(error));

    SavedGraphLayout loaded;
    QVERIFY2(loadGraphLayout(layoutFilename, *g_assemblyGraph, &loaded, &error),
             qPrintable(error));
    QCOMPARE(loaded.doubleMode, original.doubleMode);
    QCOMPARE(loaded.nodePoints, original.nodePoints);

    g_settings->arrowheadsInSingleMode = 1.0;
    g_settings->doubleMode = loaded.doubleMode;
    MyGraphicsScene scene;
    g_assemblyGraph->applySavedLayout(loaded, &scene);
    for (auto it = loaded.nodePoints.cbegin(); it != loaded.nodePoints.cend(); ++it)
    {
        DeBruijnNode * node = g_assemblyGraph->m_deBruijnGraphNodes.value(it.key());
        QVERIFY(node != nullptr);
        QVERIFY(node->getGraphicsItemNode() != nullptr);
        QCOMPARE(node->getGraphicsItemNode()->m_linePoints, it.value());
        QVERIFY(node->getGraphicsItemNode()->m_hasArrow);
    }
    QVERIFY(g_assemblyGraph->m_deBruijnGraphNodes.value("1-")->getGraphicsItemNode() == nullptr);

    int singleModeEdgeCount = 0;
    for (DeBruijnEdge *edge : g_assemblyGraph->m_deBruijnGraphEdges)
    {
        if (edge->getGraphicsItemEdge() != nullptr)
            ++singleModeEdgeCount;
    }
    QVERIFY(singleModeEdgeCount > 0);

    SavedGraphLayout invalidSingleMode = original;
    invalidSingleMode.nodePoints.insert("1-", std::vector<QPointF>{QPointF(20.0, 0.0),
                                                                   QPointF(30.0, 0.0)});
    QVERIFY(!saveGraphLayout(layoutFilename, *g_assemblyGraph, invalidSingleMode, &error));
    QVERIFY(error.contains("both orientations", Qt::CaseInsensitive));

    SavedGraphLayout doubleMode;
    doubleMode.doubleMode = true;
    doubleMode.nodePoints.insert("1+", std::vector<QPointF>{QPointF(0.0, 0.0),
                                                            QPointF(10.0, 0.0)});
    doubleMode.nodePoints.insert("1-", std::vector<QPointF>{QPointF(0.0, 20.0),
                                                            QPointF(10.0, 20.0)});
    doubleMode.nodePoints.insert("12+", std::vector<QPointF>{QPointF(30.0, 0.0),
                                                             QPointF(40.0, 0.0)});
    doubleMode.nodePoints.insert("12-", std::vector<QPointF>{QPointF(30.0, 20.0),
                                                             QPointF(40.0, 20.0)});
    QVERIFY2(saveGraphLayout(layoutFilename, *g_assemblyGraph, doubleMode, &error),
             qPrintable(error));
    QVERIFY2(loadGraphLayout(layoutFilename, *g_assemblyGraph, &loaded, &error),
             qPrintable(error));
    QVERIFY(loaded.doubleMode);
    QCOMPARE(loaded.nodePoints, doubleMode.nodePoints);

    g_settings->doubleMode = true;
    g_assemblyGraph->applySavedLayout(loaded, &scene);
    for (auto it = loaded.nodePoints.cbegin(); it != loaded.nodePoints.cend(); ++it)
    {
        DeBruijnNode *node = g_assemblyGraph->m_deBruijnGraphNodes.value(it.key());
        QVERIFY(node != nullptr);
        QVERIFY(node->getGraphicsItemNode() != nullptr);
        QCOMPARE(node->getGraphicsItemNode()->m_linePoints, it.value());
        QVERIFY(node->getGraphicsItemNode()->m_hasArrow);
    }

    QFile jsonFile(layoutFilename);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));
    const QJsonObject validRoot = QJsonDocument::fromJson(jsonFile.readAll()).object();
    jsonFile.close();
    QCOMPARE(validRoot.value("version").toInt(), 2);

    auto writeRoot = [&jsonFile, &layoutFilename](const QJsonObject &root) {
        jsonFile.setFileName(layoutFilename);
        if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        const bool written = jsonFile.write(QJsonDocument(root).toJson(QJsonDocument::Compact)) >= 0;
        jsonFile.close();
        return written;
    };

    SavedGraphLayout unchanged;
    unchanged.doubleMode = false;
    unchanged.nodePoints.insert("sentinel", std::vector<QPointF>{QPointF(), QPointF(1.0, 1.0)});
    const int preservedSceneItemCount = scene.items().size();

    QVERIFY(jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QVERIFY(jsonFile.write("{not valid JSON") > 0);
    jsonFile.close();
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("Invalid JSON", Qt::CaseInsensitive));
    QCOMPARE(scene.items().size(), preservedSceneItemCount);
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QJsonObject legacyRoot = validRoot;
    legacyRoot.insert("version", 1);
    QVERIFY(writeRoot(legacyRoot));
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("version 2", Qt::CaseInsensitive));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QJsonObject wrongDimensionsRoot = validRoot;
    QJsonObject wrongDimensions = wrongDimensionsRoot.value("graph").toObject();
    wrongDimensions.insert("nodeCount", wrongDimensions.value("nodeCount").toInt() + 1);
    wrongDimensionsRoot.insert("graph", wrongDimensions);
    QVERIFY(writeRoot(wrongDimensionsRoot));
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("dimensions", Qt::CaseInsensitive));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QJsonObject unknownNodeRoot = validRoot;
    QJsonObject unknownNodes = unknownNodeRoot.value("nodes").toObject();
    unknownNodes.insert("missing+", QJsonArray{QJsonArray{0.0, 0.0},
                                               QJsonArray{1.0, 1.0}});
    unknownNodeRoot.insert("nodes", unknownNodes);
    QVERIFY(writeRoot(unknownNodeRoot));
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("missing+"));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QJsonObject shortLineRoot = validRoot;
    QJsonObject shortLineNodes = shortLineRoot.value("nodes").toObject();
    QJsonArray onePoint;
    QJsonArray oneCoordinate;
    oneCoordinate.append(0.0);
    oneCoordinate.append(0.0);
    onePoint.append(oneCoordinate);
    shortLineNodes.insert("1+", onePoint);
    shortLineRoot.insert("nodes", shortLineNodes);
    QVERIFY(writeRoot(shortLineRoot));
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY2(error.contains("fewer than two", Qt::CaseInsensitive), qPrintable(error));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QJsonObject invalidCoordinateRoot = validRoot;
    QJsonObject invalidCoordinateNodes = invalidCoordinateRoot.value("nodes").toObject();
    QJsonArray invalidPoints = invalidCoordinateNodes.value("1+").toArray();
    QJsonArray invalidCoordinate = invalidPoints[0].toArray();
    invalidCoordinate[0] = "not-a-number";
    invalidPoints[0] = invalidCoordinate;
    invalidCoordinateNodes.insert("1+", invalidPoints);
    invalidCoordinateRoot.insert("nodes", invalidCoordinateNodes);
    QVERIFY(writeRoot(invalidCoordinateRoot));
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("exactly two numbers", Qt::CaseInsensitive));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QJsonObject emptyNodesRoot = validRoot;
    emptyNodesRoot.insert("nodes", QJsonObject());
    QVERIFY(writeRoot(emptyNodesRoot));
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("no visible nodes", Qt::CaseInsensitive));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QJsonObject singleConflictRoot = validRoot;
    QJsonObject singleDisplay = singleConflictRoot.value("display").toObject();
    singleDisplay.insert("doubleMode", false);
    singleConflictRoot.insert("display", singleDisplay);
    QVERIFY(writeRoot(singleConflictRoot));
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("both orientations", Qt::CaseInsensitive));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    QVERIFY(writeRoot(validRoot));

    // A layout belongs to one exact graph.  Rejection must also leave the
    // caller's existing result untouched.  The sequence check distinguishes
    // graphs that otherwise have identical names, lengths and topology.
    DeBruijnNode * changedNode = g_assemblyGraph->m_deBruijnGraphNodes.value("1+");
    QVERIFY(changedNode != nullptr);
    QByteArray changedSequence = changedNode->getSequence();
    QVERIFY(!changedSequence.isEmpty());
    changedSequence[0] = changedSequence[0] == 'A' ? 'C' : 'A';
    changedNode->setSequence(changedSequence);
    QVERIFY(!loadGraphLayout(layoutFilename, *g_assemblyGraph, &unchanged, &error));
    QVERIFY(error.contains("fingerprint", Qt::CaseInsensitive));
    QVERIFY(unchanged.nodePoints.contains("sentinel"));

    scene.clear();
    createGlobals();
    QTemporaryFile edgeGraphFile(QDir::tempPath() + "/bandage-layout-edges-XXXXXX.gfa");
    QVERIFY(edgeGraphFile.open());
    QTextStream edgeGraphOut(&edgeGraphFile);
    edgeGraphOut << "H\tVN:Z:1.0\n";
    edgeGraphOut << "S\tA\tAAAA\n";
    edgeGraphOut << "S\tB\tCCCC\n";
    edgeGraphOut << "L\tA\t+\tB\t+\t1M\n";
    edgeGraphOut << "L\tA\t+\tA\t+\t1M\n";
    edgeGraphOut << "L\tB\t+\tB\t-\t1M\n";
    edgeGraphOut.flush();
    edgeGraphFile.close();
    QVERIFY(g_assemblyGraph->loadGraphFromFile(edgeGraphFile.fileName()));

    SavedGraphLayout edgeLayout;
    edgeLayout.doubleMode = false;
    edgeLayout.nodePoints.insert("A+", std::vector<QPointF>{QPointF(0.0, 0.0),
                                                            QPointF(10.0, 0.0)});
    edgeLayout.nodePoints.insert("B+", std::vector<QPointF>{QPointF(30.0, 0.0),
                                                            QPointF(40.0, 0.0)});
    g_settings->doubleMode = false;
    g_assemblyGraph->applySavedLayout(edgeLayout, &scene);
    DeBruijnEdge *normalEdge = getEdgeFromNodeNames("A+", "B+");
    DeBruijnEdge *selfEdge = getEdgeFromNodeNames("A+", "A+");
    DeBruijnEdge *reverseEdge = getEdgeFromNodeNames("B+", "B-");
    QVERIFY(normalEdge != nullptr);
    QVERIFY(selfEdge != nullptr);
    QVERIFY(reverseEdge != nullptr);
    QVERIFY(normalEdge->getGraphicsItemEdge() != nullptr);
    QVERIFY(selfEdge->getGraphicsItemEdge() != nullptr);
    QVERIFY(reverseEdge->getGraphicsItemEdge() != nullptr);
    QVERIFY(!normalEdge->getGraphicsItemEdge()->path().isEmpty());
    QVERIFY(!selfEdge->getGraphicsItemEdge()->path().isEmpty());
    QVERIFY(!reverseEdge->getGraphicsItemEdge()->path().isEmpty());

    scene.clear();
    createGlobals();
    MainWindow window;
    QAction *loadLayoutAction = window.findChild<QAction *>("actionLoad_layout");
    QAction *saveLayoutAction = window.findChild<QAction *>("actionSave_layout");
    QVERIFY(loadLayoutAction != nullptr);
    QVERIFY(saveLayoutAction != nullptr);
    QVERIFY(!loadLayoutAction->isEnabled());
    QVERIFY(!saveLayoutAction->isEnabled());
    window.setUiState(GRAPH_LOADED);
    QVERIFY(loadLayoutAction->isEnabled());
    QVERIFY(!saveLayoutAction->isEnabled());
    window.setUiState(GRAPH_DRAWN);
    QVERIFY(loadLayoutAction->isEnabled());
    QVERIFY(saveLayoutAction->isEnabled());
}


void BandageTests::bedAnnotations()
{
    createGlobals();

    QTemporaryFile graphFile(QDir::tempPath() + "/bandage-bed-graph-XXXXXX.gfa");
    QVERIFY(graphFile.open());
    QTextStream graphOut(&graphFile);
    graphOut << "H\tVN:Z:1.0\n";
    graphOut << "S\tA\t" << QString(100, 'A') << "\n";
    graphOut << "S\tB\t" << QString(80, 'C') << "\n";
    graphOut.flush();
    graphFile.close();
    QVERIFY(g_assemblyGraph->loadGraphFromFile(graphFile.fileName()));

    QTemporaryFile bedFile(QDir::tempPath() + "/bandage-bed-annotations-XXXXXX.bed");
    QVERIFY(bedFile.open());
    QTextStream bedOut(&bedFile);
    bedOut << "# BED3-BED12 parser coverage\n";
    bedOut << "A\t10\t50\tfeature\t900\t+\t15\t40\t1,2,3\t2\t10,5,\t0,30,\n";
    bedOut << "A\t20\t30\treverse\t0\t-\n";
    bedOut << "A\t95\t110\tout-of-range\n";
    bedOut << "missing\t0\t10\tunmatched\n";
    bedOut << "A\tbad\t20\tmalformed\n";
    bedOut.flush();
    bedFile.close();

    const BedLoadResult result = parseBedAnnotationsFile(bedFile.fileName(), *g_assemblyGraph);
    QVERIFY(result.hasValidAnnotations());
    QCOMPARE(result.validCount, 2);
    QCOMPARE(result.malformedCount, 1);
    QCOMPARE(result.unmatchedCount, 1);
    QCOMPARE(result.outOfRangeCount, 1);
    QCOMPARE(result.skippedCount(), 3);
    QVERIFY(result.warningSummary().contains("line"));

    DeBruijnNode *positive = g_assemblyGraph->m_deBruijnGraphNodes.value("A+");
    DeBruijnNode *negative = g_assemblyGraph->m_deBruijnGraphNodes.value("A-");
    QVERIFY(positive != nullptr);
    QVERIFY(negative != nullptr);
    QCOMPARE(result.annotations.value(positive).size(), 1);
    QCOMPARE(result.annotations.value(negative).size(), 1);
    const BedAnnotation feature = result.annotations.value(positive).first();
    QCOMPARE(feature.start, qint64(10));
    QCOMPARE(feature.end, qint64(50));
    QCOMPARE(feature.thickStart, qint64(15));
    QCOMPARE(feature.thickEnd, qint64(40));
    QCOMPARE(feature.colour, QColor(1, 2, 3));
    QCOMPARE(feature.blocks.size(), 2);
    QCOMPARE(feature.blocks[1].start, qint64(40));
    QCOMPARE(feature.blocks[1].end, qint64(45));

    replaceBedAnnotations(*g_assemblyGraph, result);
    QCOMPARE(positive->getBedAnnotations().size(), 1);
    QCOMPARE(negative->getBedAnnotations().size(), 1);

    QTemporaryFile invalidFile(QDir::tempPath() + "/bandage-bed-invalid-XXXXXX.bed");
    QVERIFY(invalidFile.open());
    QTextStream invalidOut(&invalidFile);
    invalidOut << "missing\t0\t10\n";
    invalidOut.flush();
    invalidFile.close();
    const BedLoadResult invalidResult = parseBedAnnotationsFile(invalidFile.fileName(), *g_assemblyGraph);
    QVERIFY(!invalidResult.hasValidAnnotations());
    QVERIFY(!invalidResult.fatalError.isEmpty());
    QCOMPARE(positive->getBedAnnotations().size(), 1); // A failed load is transactional.

    {
        std::vector<QPointF> points{QPointF(0.0, 0.0), QPointF(100.0, 0.0)};
        GraphicsItemNode item(positive, points);
        QVERIFY(!item.makePartialPath(0.1, 0.5).isEmpty());
        item.setNodeColour();
        QImage rendered(120, 40, QImage::Format_ARGB32_Premultiplied);
        rendered.fill(Qt::transparent);
        QPainter painter(&rendered);
        painter.translate(10.0, 20.0);
        item.paint(&painter, nullptr, nullptr);
        painter.end();
        QCOMPARE(rendered.pixelColor(30, 20), QColor(1, 2, 3));
        QCOMPARE(rendered.pixelColor(85, 20), QColor(63, 160, 230));

        MainWindow window;
        window.setUiState(GRAPH_LOADED);
        QPushButton *loadButton = window.findChild<QPushButton *>("bedLoadButton");
        QPushButton *clearButton = window.findChild<QPushButton *>("bedClearButton");
        QCheckBox *intervalCheckBox = window.findChild<QCheckBox *>("bedIntervalCheckBox");
        QVERIFY(loadButton != nullptr);
        QVERIFY(clearButton != nullptr);
        QVERIFY(intervalCheckBox != nullptr);
        QVERIFY(loadButton->isEnabled());
        QVERIFY(!clearButton->isEnabled());
        window.m_bedFileName = "test.bed";
        window.m_bedValidCount = 2;
        window.m_bedSkippedCount = 3;
        window.updateBedControls();
        QVERIFY(clearButton->isEnabled());
        QVERIFY(intervalCheckBox->isEnabled());

        ::clearBedAnnotations(*g_assemblyGraph);
        QVERIFY(!positive->hasBedAnnotations());
        QVERIFY(!negative->hasBedAnnotations());
    }

    createGlobals();
    QVERIFY(g_assemblyGraph->loadGraphFromFile(getTestDirectory() + "test_plasmids.gfa"));
    const BedLoadResult example = parseBedAnnotationsFile(
        getTestDirectory() + "bed_annotations_example.bed", *g_assemblyGraph);
    QVERIFY2(example.hasValidAnnotations(), qPrintable(example.fatalError));
    QCOMPARE(example.validCount, 4);
    QCOMPARE(example.skippedCount(), 0);
}


QTEST_MAIN(BandageTests)
