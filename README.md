# WordWrapper
Jeevan Vasanthan and Spandan Parikh
(enter your netID) & (snp131)

The program we wrote, ww.c, contains three methods: wrap(int width, int fd_input, fd_output), isFileOrDir(char *name), and main(int argc, char argv[]).

isFileOrDir(char *name) is a helper method that takes in the inputted file name. Utilizing S_ISDIR and S_ISREG to determine whether it's a file (returns 1), or 
a directory(returns 2), and returns 0 otherwise.

The main() method does some of the error checking, such as the width parameter being valid (> 0), if there is a file name/path inputted, the main method calls 
on to the isFileOrDir(). If it's determined to be a file, the file is opened and the wrap() is called to "normalize" file. If it's determined to be a directory,
then it opens and changes to the inputed directory, compares the file name with "wrap." to determine if wrapping should be allowed. If it's allowed, the file 
is opened to read and the wrap() method is called to "normalize" the file. Then a new file is written into. If neither than it takes in the standard input and 
calling wrap ...

TEST CASES:
Test Case 0: Tests with no more than one whitespace between words and without paragraph breaks.

		./ww 20 test0.txt > test0_out
		Input:
		This is very good text that has been put into a file for the purposes
		of being
		an
		example.
		
		That       is
       			good.

		Output:
		This is very good
		text that has been
		put into a file for
		the purposes of
		being an example.

		That is good.
Test Case 1: Tests with an input with more than one whitespace between words and paragraph breaks with multiple blank lines.

        	./ww 20 test1.txt > test1_out
		Input:
		This is very good text that has been put into a file for the purposes
		of being
		an
		example.
		

		That       is
       			good.

		Output:
		This is very good
		text that has been
		put into a file for
		the purposes of
		being an example.

		That is good.

Test Case 2: Tests with an input with no blank lines between paragraphs.

        	./ww 20 test2.txt > test2_out
		Input:
		This is very good text that has been put into a file for the purposes
		of being
		an
		example.

		Output:
		This is very good
		text that has been
		put into a file for
		the purposes of
		being an example.

		That is good.

Test Case 3: Tests with an input with characters equal to width size.
        	
		./ww 20 test3.txt > test4_out
        	Input:
        	ttttt ttttt ttttt tt

        	Output:
        	ttttt ttttt ttttt tt

Test Case 4: Tests with an input with multiple words > width size
        	
		./ww 20 test4.txt > test4_out
        	Input:
		Good evening, this is a particularly interesting project 
		called WordWrapperinSystemProgramming
		and it is goind well thus far. Created by
		SpandanParikhandJeevanVasanthan. 
		Have a good day.

        	Output:


Test Case 5: Tests with an input that has whitespace characters at the beginning of a new line
		
		./ww 20 test5.txt > test5_out
		Input:
		    This is very good text that has been put into a file for the purposes
				of being
				    an
				example.

		Output:


Test Case 6: Tests with an input of a different width length
		
		./ww 30 test6.txt > test6_out
		Input:
		This is very good text that has been put into a file for the purposes
			of being
			an
			example.

			That       is
				good.

		Output:
		This is very good text that
		has been put into a file for
		the purposes of being an
		example.

		That is good.
