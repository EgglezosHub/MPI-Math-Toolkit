# MPI-Math-Toolkit (Tool 1)
<h2>Overview</h2>
<p>
<b>Math Toolkit MPI</b> is a parallel computing application implemented using the MPI (Message Passing Interface) standard.
Its main functionality is to determine whether a given integer sequence is sorted, and if not, identify the position of the first element that breaks the sorting order.
</p>

<p>
The program distributes the sequence across multiple MPI processes, performs comparisons in parallel, and collects the results on the root process for reporting.
</p>

<hr>

<h2>Features</h2>
<ul>
<li>✅ Checks if a sequence of integers is sorted.</li>
<li>✅ Identifies the first unsorted element’s position (if any).</li>
<li>✅ Evenly distributes computation across MPI processes, handling cases when the sequence length is not divisible by the number of processes.</li>
<li>✅ Ensures that boundary elements between sub-arrays are also correctly compared.</li>
<li>✅ Interactive menu to test multiple sequences without restarting the program.</li>
</ul>

<hr>

<h2>How it Works</h2>
<ol>
<li>The root process (<code>rank 0</code>) reads the sequence from the user.</li>
<li>It splits the sequence into roughly equal chunks and sends them to all processes.
    <ul>
    <li>If the sequence length <code>N</code> is not a multiple of the number of processes <code>p</code>, the surplus elements are distributed so that no process differs by more than one element from others.</li>
    </ul>
</li>
<li>Each process checks if its sub-sequence is sorted and also verifies that its first element is ≥ the last element of the previous process (to ensure global ordering).</li>
<li>All processes send their findings back to the root process.</li>
<li>The root process combines the results, determines if the overall sequence is sorted, and if not, displays the position of the first unsorted element.</li>
</ol>

<hr>

<h2>Installation</h2>
<p>
You need an MPI implementation (such as <a href="https://www.open-mpi.org/">OpenMPI</a> or <a href="https://www.mpich.org/">MPICH</a>) and a C compiler.
</p>

<pre><code>sudo apt install mpich
</code></pre>

<hr>

<h2>Building</h2>
<p>Compile the program with:</p>

<pre><code>mpicc -o math_toolkit_mpi MathTool1.c
</code></pre>

<hr>

<h2>Running</h2>
<p>Run the program with <code>mpirun</code> or <code>mpiexec</code> specifying the number of processes.
For example, to run with 4 processes:</p>

<pre><code>mpirun -np 4 ./math_toolkit_mpi
</code></pre>

<hr>

<h2>Usage</h2>
<p>Once running, the root process will prompt you:</p>
<ul>
<li>Enter <code>N</code>: the length of the sequence.</li>
<li>Enter each element of the sequence.</li>
<li>The program will report if the sequence is sorted.</li>
<li>You can choose:
    <ul>
    <li><code>1</code> to continue with another sequence.</li>
    <li><code>2</code> to exit.</li>
    </ul>
</li>
</ul>

<p>Sample output:</p>

<pre>
Enter N: 6
t[0]: 1
t[1]: 2
t[2]: 3
t[3]: 2
t[4]: 5
t[5]: 6

Array is not sorted:
-&gt; Position of first unsorted element is: 2
</pre>

<hr>

<h2>Notes</h2>
<ul>
<li>The program ensures load balancing by assigning each process a chunk size that differs by at most one element.</li>
<li>Each process receives the last element of the previous process’s chunk to correctly check cross-boundary ordering.</li>
<li>Works correctly even when <code>N % p ≠ 0</code>.</li>
</ul>

<hr>

<h2>Known Issues</h2>
<ul>
<li>Initially, the menu selection was not communicated to all processes, which caused inconsistent behavior. This has been fixed by broadcasting the choice from the root process to all others.</li>
</ul>

<hr>

<h2>Example Platforms</h2>
<p>Tested on:</p>
<ul>
<li>Ubuntu 20.04 with MPICH via Oracle VM VirtualBox.</li>
</ul>

<hr>

<br><br>

# MPI-Math-Toolkit (Tool 2)

<h2>Overview</h2>
<p>
<b>Math Toolkit MPI – Vector Statistics Tool</b> is a parallel MPI application that computes several statistical properties of a user-provided vector of real numbers. 
It distributes the vector across multiple processes, computes local and global metrics, and reports comprehensive results.
</p>

<hr>

<h2>Features</h2>
<ul>
<li>✅ Calculates mean, variance, minimum, and maximum of a vector.</li>
<li>✅ Identifies the number of elements below and above the mean.</li>
<li>✅ Constructs a normalized percentage vector (D) and finds its maximum value and position.</li>
<li>✅ Computes the prefix sums of the vector elements.</li>
<li>✅ Supports vectors where <code>n</code> is not divisible by the number of processes.</li>
<li>✅ Interactive mode: allows running multiple computations without restarting the program.</li>
</ul>

<hr>

<h2>How it Works</h2>
<ol>
<li>The root process (<code>rank 0</code>) reads the vector size and its elements from the user.</li>
<li>The vector size is broadcasted to all processes.</li>
<li>The vector is partitioned among processes using <code>MPI_Scatterv</code>:
    <ul>
        <li>Handles uneven distribution when <code>n % p ≠ 0</code> by assigning the remainder to the last process.</li>
    </ul>
</li>
<li>Each process computes local sums, minima, maxima, and variance contributions.</li>
<li>Global sums, minima, maxima, and variance are computed using <code>MPI_Reduce</code>.</li>
<li>The mean is computed at the root and broadcasted to all processes.</li>
<li>Each process computes a local normalized percentage vector <code>D</code> (relative to the global min and max).</li>
<li>The maximum value of <code>D</code> and its position are determined using <code>MPI_MAXLOC</code>.</li>
<li>Prefix sums are computed with <code>MPI_Scan</code> and gathered with <code>MPI_Gatherv</code>.</li>
<li>The root process displays:
    <ul>
        <li>Vector <code>X</code></li>
        <li>Mean, variance, min, max</li>
        <li>Counts of elements below and above the mean</li>
        <li>Vector <code>D</code> and its maximum value and position</li>
        <li>Prefix sums</li>
    </ul>
</li>
<li>The user is prompted to continue or exit.</li>
</ol>

<hr>

<h2>Installation</h2>
<p>
You need an MPI implementation (such as <a href="https://www.open-mpi.org/">OpenMPI</a> or <a href="https://www.mpich.org/">MPICH</a>) and a C compiler.
</p>

<pre><code>sudo apt install mpich
</code></pre>

<hr>

<h2>Building</h2>
<p>Compile the program with:</p>

<pre><code>mpicc -o vector_stats_mpi MathTool2.c
</code></pre>

<hr>

<h2>Running</h2>
<p>Run the program with <code>mpirun</code> or <code>mpiexec</code> specifying the number of processes.  
For example, to run with 4 processes:</p>

<pre><code>mpirun -np 4 ./vector_stats_mpi
</code></pre>

<hr>

<h2>Usage</h2>
<p>Once running, the root process will prompt you:</p>
<ul>
<li>Enter <code>n</code>: the length of the vector.</li>
<li>Enter each element of the vector.</li>
<li>The program will compute and display results.</li>
<li>You can choose:
    <ul>
    <li><code>1</code> to continue with another vector.</li>
    <li><code>0</code> to exit.</li>
    </ul>
</li>
</ul>

<p>Sample output highlights:</p>

<pre>
X:             [1.0000, 2.0000, 3.0000, 4.0000]
Average:       2.5000
Xmin:          1.0000
Xmax:          4.0000
Below Average: 2
Above Average: 2
Variance:      1.2500
D:             [0.0000, 33.3333, 66.6667, 100.0000]
Dmax:          100.0000
Dmaxloc:       3
Prefix Sums:   [1.0000, 3.0000, 6.0000, 10.0000]
</pre>

<hr>

<h2>Notes</h2>
<ul>
<li>The program ensures correct results even when <code>n</code> is not a multiple of <code>p</code> by assigning the remainder of elements to the last process.</li>
<li>Uses <code>MPI_Scatterv</code> and <code>MPI_Gatherv</code> for flexible data distribution and collection.</li>
<li>Handles distributed calculation of complex metrics like normalized vectors, maxima with positions, and prefix sums.</li>
</ul>

<hr>

<h2>Known Issues</h2>
<ul>
<li>Currently, the uneven partitioning simply assigns all remaining elements to the last process, which may lead to slight imbalance.</li>
</ul>

<hr>

<h2>Example Platforms</h2>
<p>Tested on:</p>
<ul>
<li>Ubuntu 20.04 with MPICH via Oracle VM VirtualBox.</li>
</ul>

<hr>

<br>
<h2>Author</h2>
<p>
📄 <b>Dimitrios Egglezos</b><br>
</p>

<hr>

<h2>License</h2>
<p>
This project is for educational purposes and distributed as-is without any warranty.
</p>
