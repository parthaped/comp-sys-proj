#!/usr/bin/env python3
"""Generate self-contained Overleaf-ready LaTeX for ECE 434 Project 1."""
from pathlib import Path
import textwrap

ROOT = Path(__file__).resolve().parent

FILES = [
    ("common.h", "C"),
    ("data_gen.h", "C"),
    ("data_gen.c", "C"),
    ("metrics.h", "C"),
    ("metrics.c", "C"),
    ("wait_status.h", "C"),
    ("wait_status.c", "C"),
    ("trace.h", "C"),
    ("trace.c", "C"),
    ("tree_proc.h", "C"),
    ("tree_proc.c", "C"),
    ("main.c", "C"),
    ("Makefile", "make"),
]


def escape_lstlisting_end(s: str) -> str:
    if "\\end{lstlisting}" in s:
        raise ValueError("Source contains literal \\end{lstlisting}")
    return s


def main() -> None:
    parts = []
    parts.append(
        textwrap.dedent(
            r"""
            % =============================================================================
            % ECE 434 Sp26 — Single-file LaTeX report (upload to Overleaf as one .tex)
            % Compiler: pdfLaTeX (Menu: Overleaf default). No shell-escape or minted required.
            % To regenerate from repo sources:  python3 gen_latex_report.py
            % =============================================================================

            \documentclass[11pt,a4paper]{article}

            \usepackage[T1]{fontenc}
            \usepackage[utf8]{inputenc}
            \usepackage{lmodern}
            \usepackage{geometry}
            \geometry{margin=1in}
            \usepackage{xcolor}
            \usepackage{listings}
            \usepackage{caption}
            \usepackage{booktabs}
            \usepackage[hidelinks]{hyperref}
            \usepackage{nameref}

            \definecolor{codebg}{RGB}{248,248,255}
            \definecolor{codeframe}{RGB}{120,120,120}
            \definecolor{keywords}{RGB}{0,0,139}
            \definecolor{comments}{RGB}{106,115,125}

            \lstdefinestyle{textbookc}{
              language=C,
              basicstyle=\ttfamily\footnotesize,
              keywordstyle=\color{keywords}\bfseries,
              commentstyle=\color{comments}\itshape,
              stringstyle=\color{teal},
              numberstyle=\tiny\sffamily\color{gray},
              numbers=left,
              stepnumber=1,
              numbersep=8pt,
              frame=single,
              framerule=0.4pt,
              rulecolor=\color{codeframe},
              backgroundcolor=\color{codebg},
              breaklines=true,
              breakatwhitespace=false,
              showstringspaces=false,
              tabsize=4,
              keepspaces=true,
              columns=fullflexible,
              aboveskip=\smallskipamount,
              belowskip=\smallskipamount,
              captionpos=b,
            }
            \lstdefinestyle{textbookmake}{
              language=make,
              basicstyle=\ttfamily\footnotesize,
              commentstyle=\color{comments}\itshape,
              numberstyle=\tiny\sffamily\color{gray},
              numbers=left,
              stepnumber=1,
              numbersep=8pt,
              frame=single,
              framerule=0.4pt,
              rulecolor=\color{codeframe},
              backgroundcolor=\color{codebg},
              breaklines=true,
              tabsize=4,
              keepspaces=true,
              columns=fullflexible,
              aboveskip=\smallskipamount,
              belowskip=\smallskipamount,
              captionpos=b,
            }
            \lstset{style=textbookc}

            \title{\textbf{Project 1: LINUX OS, Processes and Inter Process Communication}\\
            \large Rutgers ECE 434 / 579 --- Spring 2026}
            \author{Course Project (Group Submission)}
            \date{\today}

            \begin{document}
            \maketitle
            \tableofcontents
            \clearpage

            \section{Problem Overview}
            This project implements a multi-process solution on Linux that (1)~generates or loads a text file of at least $L \geq 12{,}000$ positive integers with 150 hidden keys (random integers in $[-100,-1]$) placed at unknown positions; (2)~partitions work across up to $PN \leq 50$ processes arranged in a tree with branching factor $B \in \{2,3,5\}$; (3)~uses \textbf{pipes} for IPC, with parents collecting child results using \textbf{poll()} to avoid fixed-order blocking; (4)~uses \textbf{waitpid()} and status macros to explain how each child terminated; (5)~records per-process timing and bytes transferred in a CSV trace; and (6)~invokes \texttt{pstree -p} at key points for validation.

            \section{Design Summary}
            The implementation loads the file into a heap array in the root and relies on copy-on-write semantics after \texttt{fork()} so children can read partitions without rereading the file. A \emph{plan} assigns each node a contiguous index range $[lo,hi)$, a unique exit argument, and subtree sizes so that exactly $PN$ processes participate. Leaves compute local max, sum/count (for average), and detect hidden keys. Internal nodes merge child \texttt{SegmentResult} structures after reading whichever pipe becomes ready first. Optional \texttt{poll} timeout triggers \texttt{SIGKILL} for defensive termination as suggested in the handout.

            \textbf{Trade-offs.} More processes increase potential parallelism but add fork/pipe/wait overhead and smaller per-process segments (worse cache locality). Taller/wider trees increase communication volume proportional to the number of internal nodes. Timings for $L \in \{12000, 100000, 1000000\}$ should be measured on Linux; fill Table~\ref{tab:timing} with your machine's results.

            \begin{table}[ht]
              \centering
              \caption{Wall-clock time (seconds) --- replace with measured values on your Linux host.}
              \label{tab:timing}
              \begin{tabular}{@{}lccc@{}}
                \toprule
                Configuration & $L=12{,}000$ & $L=100{,}000$ & $L=1{,}000{,}000$ \\
                \midrule
                $PN=1$ (baseline) & --- & --- & --- \\
                $PN=8$, $B=2$ & --- & --- & --- \\
                $PN=8$, $B=5$ & --- & --- & --- \\
                $PN=32$, $B=2$ & --- & --- & --- \\
                \bottomrule
              \end{tabular}
            \end{table}

            For the required graph, export the table columns to a spreadsheet or Python/MATLAB and plot time versus $PN$ or versus branching factor.

            \section{Build and Run (Linux)}
            \begin{lstlisting}[style=textbookmake, numbers=none, frame=shadowbox, caption={Typical commands}]
            make clean && make
            ./ece434_proj1 12000 10 8 2 --generate --sleep 1 input.txt output.txt
            \end{lstlisting}
            Arguments: \texttt{L H PN BRANCH input output}; use \texttt{--generate [SEED]} to build the input file; \texttt{--sleep SEC} slows leaves for \texttt{pstree} observation. Output text is written to \texttt{output.txt}; trace rows append to \texttt{process_trace.csv}.

            \section{Source Listings}
            The following listings mirror the project source files line-for-line. Style follows common textbook conventions: left rule, numbered lines, monospace body.
            """
        ).strip()
    )

    label_safe = lambda n: n.replace(".", "_")

    for fname, lang in FILES:
        path = ROOT / fname
        text = escape_lstlisting_end(path.read_text(encoding="utf-8"))
        style = "textbookc" if lang == "C" else "textbookmake"
        cap = fname.replace("_", r"\_")
        parts.append(rf"\subsection{{\texttt{{{fname}}}}}")
        parts.append(
            rf"\lstset{{style={style}}}\begin{{lstlisting}}[caption={{File \texttt{{{cap}}}}},label=lst:{label_safe(fname)}]"
        )
        parts.append(text.rstrip("\n"))
        parts.append(r"\end{lstlisting}\clearpage")

    parts.append(
        textwrap.dedent(
            r"""
            \section{Answers to Additional Questions (Handout)}

            \paragraph{(1) Root killed with \texttt{kill -KILL <pid>}.}
            Signal \texttt{SIGKILL} cannot be caught or ignored. The root terminates immediately without flushing stdio buffers or cleaning up children explicitly. Child processes become orphaned and are reparented to \texttt{init} (or a subreaper such as \texttt{systemd}), which will eventually reap them when they exit. Pipes may lack a reader/writer if the kill order leaves an endpoint open; peers may see \texttt{SIGPIPE} or \texttt{EPIPE} on write, or \texttt{read} returning 0. No graceful merge of partial results occurs.

            \paragraph{(2) \texttt{pstree} with root \texttt{getpid()} vs using the wrong PID.}
            \texttt{pstree -p <pid>} shows the subtree rooted at the process with that PID. If you pass your shell's PID by mistake, you see the shell's descendants (often unrelated to your program). If you pass a child's PID, you see only that child's subtree, not siblings or ancestors. Always pass the intended process's PID (typically the project root after \texttt{fork} waves begin, often the same as the original \texttt{main} PID unless the program double-forks or daemonizes).

            \paragraph{(3) Maximum \emph{random tree} and why.}
            The assignment caps $PN \leq 50$. The implementation also uses \texttt{PlanNode plan[MAX\_PN]} with \texttt{MAX\_PN = 50}. Beyond that, process limits (\texttt{/proc/sys/kernel/pid\_max}, \texttt{ulimit -u}), memory for pipe buffers and stacks, and the operating system's ability to schedule many runnable tasks bound practical depth/breadth. Exit statuses are only 8~bits; the code assigns distinct logical exit arguments per node and masks with \texttt{\& 0xFF} when calling \texttt{exit()}---for $\leq 50$ processes this stays within distinct low-byte values by construction, but for very large trees you would need a different reporting channel than the exit status alone.

            \section{References and Course Links}
            \begin{itemize}
              \item \url{https://www.tutorialspoint.com/cprogramming/c_command_line_arguments.htm}
              \item \url{https://www.cprogramming.com/tutorial/c-tutorial.html}
              \item \url{https://www.gnu.org/software/libc/manual/html_node/Pipes-and-FIFOs.html}
              \item \url{https://www.gnu.org/software/libc/manual/html_node/Creating-a-Process.html}
              \item \url{https://www.gnu.org/software/libc/manual/html_node/Process-Completion.html}
              \item \url{https://en.wikipedia.org/wiki/Depth-first_search}
              \item \texttt{man 2 pipe}, \texttt{man 2 poll}, \texttt{man 2 waitpid}, \texttt{man 1 pstree}
            \end{itemize}

            \end{document}
            """
        ).strip()
    )

    out_path = ROOT / "ECE434_Project1_Linux_IPC_Report.tex"
    out_path.write_text("\n\n".join(parts), encoding="utf-8")
    print("Wrote", out_path)


if __name__ == "__main__":
    main()
