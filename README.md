# IN104-Creating your own language : UF-C

This project contains the lexer, parser and interpreter for our own langage: UF-C


## Presentation

The UF-C language is an esoteric programming programming language based on combat sports, so that a piece of code reads like fighters preparing for a fight and/or actually fighting.

The interpreter for this language was made in C, hence the name.


## How to write in UF-C

### Comments

Comments can be made as in C with `/*` to begin the comment and `*/` to end it.

### Code structure

A UF-C file is always separated into two parts
- A defintions phase
- A main phase

A code always starts by the definitions: all the variables and functions must be defined and initialized before any other command is executed.

A variable is a fighter in UF-C, and a function definition is a training regimen.

So the definitions and initializations of variables correspond to the presentation of the fighters, while function definitions are the fighter's training.

The definition phase is separated from the main phase by the sentence `And the competition begins`.

### Variable defintion

This can be done using a few key sentences

- `Rose has an IQ of 3.5`: declares a new floating point variable named `Rose` intialized with value 3.5
- `Jack has this number of fans: 5`: declares a new integer varaible named `Jack` initializes with value 5

The idea in the rest of the code is that a float variable is smart and an int variable is famous.

### Function defintion

The functions correspond to training, and has the following syntax

    Func is starting their training with the smart Rose and the famous Jack to increase their IQ:
        /* Function body */
        Rose is thrown out
    training is over

Which creates a function named `Func` with the float `Rose` and the integer `Jack` as input, and returns a float (comes from `to increase their IQ`).
The returned float is the value of `Rose` at the end of the function body (since `Rose is thrown out`).

A function always ends with the key sentence `training is over`.

It is possible for the function to return no variable (type `void` in C) by using the keyword `effectiveness`, and to take in no argument with the keyword `noone`.

Another example of a function that takes no argument and returns nothing

    Func is starting their training with noone to increase their effectiveness:
        /* Function body */
    training is over
    
### Basic operations

The basic operations have been implemented as follow

- `Rose hits Mark`: assign the value of `Rose` to `Mark` (in C: `Mark = Rose`)
- `Mark joins Rose and hits James`: adds up the values of `Mark` and `Rose` and stores the result in `James` (in C: `James = Mark + Rose`)
- `Mark tosses away Rose and hits James`: removes the value of `Rose` from `Mark` and stores the result in `James` (in C: `James = Mark - Rose`)
- `Mark deals with Rose and hits James`: multiplies the values of `Mark` and `Rose` and stores the result in `James` (in C: `James = Mark * Rose`)

### Function call

A function is called using the following syntax

    F punches James with Rose and Mark

which is equivalent in C to `James = F(Rose, Mark)`

### Printing to the standard output

To print a value, the following key sentences are used

- `the ring girl shows "Text to show"`: this prints `"Text to show"` to the standard output (in C: `printf("Text to show")`)
- `the ring girl shows the wits of Rose`: this prints the value of the floating point variable `Rose` (in C: `printf("%f", Rose)`)
- `The ring girl shows the fans of Jack`: this prints the value of the integer variable `Jack` (in C: `printf("%d", Jack)`)

- `A time out is announced`: prints the character `\n`, which is the line break character

### While loops

The syntax for a while loop is as follow

    Sam beats down Rex until they come to an agreement
    Meanwhile Mark punches Rex with Jack

Which is equivalent in C to 

    while(Sam != Rex) { Rex = Mark(Jack) }

---

Another syntax possible is

    Jack beats down Rex until they give up
    meanwhile Func enrolls Rose

which means in C

    while(Rex != 0) { Func(Rose) }

In this syntax, the first variable `Jack` is useless and can be replaced by `_` or any name (the corresponding variable will be ignore by the interpreter so it doesn't need to have been defined previously)

_Notes_

- A loop is always written with two lines and can only call a function, so the body of the loop must be defined in a function beforehand
- The function corresponding to the body of the loop can (and sometimes have to) have a side effect and modify the values of varaibles outside of it's return value
- It is also possible to replace the `Rose` in the last example by the keyword `noone`, which means the function is simply called with no parameter

### Conditions (if - else)

#### Structure

A condition is structured in two phases

- First, the definitions of the unit conditions as fighting round or `Match`, identified with a number
- Second the body of the conditional structure: the functions to call if each condition is satisfied, or if none is satisfied

A condition block is of the form

    A new tournament begins :
        -Match 1: a challenges b
        -Match 2: b challenges c
    And the gambling den opens :
        -Add bets on 1 and 2 using a and b and gives the money to c
        -Sub bets on 2 using c and 24.0 and gives the money to c
        Finally Mult takes the rest of the bets using b and a and gives the money to c
    The gambling den closes

Which is equivalent in C to:

    if (a >= b && b >= c) { c = Add(a,b) }
    else if (b >= c) { c = Sub(c, 24.0) }
    else { c = Mult(b, a) }

Let's break down each part of the conditions in UF-C

#### Definition of the unit conditions

The definition of the unit conditions starts with the key sentence `A new tournament begins :`, then a line break followed by one condition definition on each line.

One unit condition definition looks like

    -Match 1: a challenges b

which gives the id `1` to the condition `a >= b`

#### Body of the conditional structure

The body starts by the key sentence `And the gambling den opens :`, then a line break followed by one condition and function call per line.

One of these line look like

    -Add bets on 1 and 2 using a and b and gives the money to c

Which means that if unit conditions `1` and `2` are true, then the function `Add` will be called with arguments `a` and `b` and it's value will be saved to `c`.

#### Else

The last line of the conditional structure is optional and corresponds to an else statement.

It has the following structure

    Finally Mult takes the rest of the bets using b and a and gives the money to c

Which means that if none of the previously defined conditions is met, the function `Mult` is called with arguments `b` and `a` and it's returned value is stored in `c`.

This transates in C to

    else { c = Mult(b,a) }

#### End of a conditional bloc

A conditional bloc always ends with the key sentence `The gambling den closes`


## Compiling the interpreter

To compile the Lexer, Parser and Interpreter and generate the UF-C interpreter executable, use the terminal to go to the main folder, followed by the command

    make UF-C


## How to execute a UF-C file

To interprete UF-C code, the code file has to be passed as an argument to UF-C (and has to have the .ufc extension).
For example

    ./UF-C in.ufc


## Examples

Following are a few pieces of code in UF-C using all of the currently implemented commands to perform various basic programming tasks.

First, a few examples of UF-C features in use
- Addition of floating numbers
- While loop
- Conditions (if - else)

Followed by two algorithms
- Factorial
- Fibonacci


### Addition of floating numbers 
    /*Definitions*/
    
    z has an IQ of 0.0
    
    F is starting their training with the smart x and the smart y to increase their IQ :
        x joins y and hits z
        z is thrown out
    training is over
    
    a has an IQ of 25.6
    b has an IQ of 25.5
    c has an IQ of 0.0
    
    And the competition begins
    
    
    /*Main*/
    
    F punches c with a and b
    
    the ring girl shows the wits of a
    the ring girl shows " + "
    the ring girl shows the wits of b
    the ring girl shows " = "
    the ring girl shows the wits of c
    
    A time out is announced


### While loop
    /*Definitions*/
    
    z has an IQ of 0.0
    b has an IQ of 15.0
    
    instructions is starting their training with noone to increase their effectiveness:
        b tosses away 1 and hits b
    training is over
    
    And the competition begins
    
    
    /*Main*/
    
    the ring girl shows "Before the loop, b = "
    the ring girl shows the wits of b
    A time out is announced
    
    _ beats down b until they give up
    meanwhile instructions enrolls noone
    
    the ring girl shows "After the loop, b = "
    the ring girl shows the wits of b
    A time out is announced


### Conditions (if - else)
    /*Definitions*/
    
    z has an IQ of 0.0
    
    a has an IQ of 25.0
    b has an IQ of 150.0
    c has an IQ of 1000.0
    
    Add is starting their training with the smart x and the smart y to increase their IQ:
        x joins y and hits z
        z is thrown out
    training is over
    
    Sub is starting their training with the smart x and the smart y to increase their IQ:
        x tosses away y and hits z
        z is thrown out
    training is over
    
    Mult is starting their training with the smart x and the smart y to increase their IQ:
        x deals with y and hits z
        z is thrown out
    training is over
    
    And the competition begins
    
    
    /*Main*/
    
    A new tournament begins :
        -Match 1: a challenges b
        -Match 2: b challenges c
    And the gambling den opens :
        -Add bets on 1 and 2 using a and b and gives the money to c
        -Sub bets on 2 using c and 24.0 and gives the money to c
        Finally Mult takes the rest of the bets using b and a and gives the money to c
    The gambling den closes
    
    the ring girl shows "z = "
    the ring girl shows the wits of z
    A time out is announced
    
    the ring girl shows "c = "
    the ring girl shows the wits of c
    A time out is announced


### Factorial
    /*Definitions*/
    
    n has this number of fans: 5
    
    
    r has this number of fans: 1
    
    Mult is starting their training with the famous x to increase their fame:
        r deals with x and hits r
        i joins 1 and hits i
    training is over
    
    i has this number of fans: 1
    
    The competition begins
    
    
    /*Main*/
    n joins 1 and hits n
    
    n beats down i until they come to an agreement
    Meanwhile Mult enrolls i
    
    n tosses away 1 and hits n
    
    The ring girl shows the fans of n
    The ring girl shows "! = "
    the ring girl shows the fans of r
    A time out is announced


### Fibonacci
    /*Definitions*/
    
    n has this number of fans: 16
    
    
    a has this number of fans: 0
    b has this number of fans: 1
    
    temp has this number of fans: 0
    
    Instruction is starting their training with the famous x to increase their fame:
        b hits temp
        a joins b and hits b
        temp hits a
    
        x joins 1 and hits x
        x is thrown out
    training is over
    
    i has this number of fans: 0
    
    The competition begins
    
    
    /*Main*/
    
    n beats down i until they come to an agreement
    Meanwhile Instruction punches i with i /*i = Instructions(i)*/
    
    The ring girl shows "Fib("
    The ring girl shows the fans of n
    The ring girl shows ") = "
    the ring girl shows the fans of a
    A time out is announced

## Credits

This language (UF-C) and corresponding interpreter was imagined and developped by Elsa Kassang and Walid Outtaleb
