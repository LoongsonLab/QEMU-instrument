//此工具记录程序执行中的追踪信息

#include "pintool.h"
#include "../string_convert.h"
#include <fstream>
#include <iostream>
#include "../ins_inspection.h"

using std::cerr;
using std::endl;
using std::string;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

std::ofstream TraceFile;
std::ofstream ExecOrderFile;
UINT32 count_trace = 0; // current trace number

/* ===================================================================== */
/* Global arguments */
/* ===================================================================== */

// 这里输出两个文件，一个记录trace中的指令，另一个记录执行顺序
string OutFilePath1 = "/home/glp/pintool_out/control_flow/trace_info.txt";
string OutFilePath2 = "/home/glp/pintool_out/control_flow/exec_order.txt";
// no_compess为真时，仅记录trace指令，不记录执行顺序
bool no_compess = 0;
// KNOB< string > KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "trace.out", "specify trace file name");
// KNOB< BOOL > KnobNoCompress(KNOB_MODE_WRITEONCE, "pintool", "no_compress", "0", "Do not compress");

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool produces a compressed (dynamic) instruction trace.\n"
            "The trace is still in textual form but repeated sequences\n"
            "of the same code are abbreviated with a number which dramatically\n"
            "reduces the output size and the overhead of the tool.\n"
            "\n";

    cerr << endl;

    return -1;
}

/* ===================================================================== */

VOID docount(const string* s) { ExecOrderFile.write(s->c_str(), s->size()); }

/* ===================================================================== */

VOID Trace(TRACE trace, VOID* v)
{
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        string traceString = "";

        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            string InsStr = INS_Disassemble(ins);
            traceString += "%" + InsStr + "\n";
        }

        // we try to keep the overhead small
        // so we only insert a call where control flow may leave the current trace

        if (no_compess)
        {
            INS_InsertCall(BBL_InsTail(bbl), IPOINT_BEFORE, AFUNPTR(docount), IARG_PTR, new string(traceString), IARG_END);
        }
        else
        {
            // Identify traces with an id
            count_trace++;

            // Write the actual trace once at instrumentation time
            string m = "@" + decstr(count_trace) + "\n";
            TraceFile.write(m.c_str(), m.size());
            TraceFile.write(traceString.c_str(), traceString.size());

            // at run time, just print the id
            string* s = new string(decstr(count_trace) + "\n");
            INS_InsertCall(BBL_InsTail(bbl), IPOINT_BEFORE, AFUNPTR(docount), IARG_PTR, s, IARG_END);
        }
    }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID* v)
{
    TraceFile << "# eof" << endl;

    TraceFile.close();
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    string trace_header = string("#\n"
                                 "# Compressed Instruction Trace Generated By Pin\n"
                                 "#\n");

    if (PIN_Init(argc, argv))
    {
        return Usage();
    }

    TraceFile.open(OutFilePath1);
    ExecOrderFile.open(OutFilePath2);
    TraceFile.write(trace_header.c_str(), trace_header.size());

    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddFiniFunction(Fini, 0);

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
