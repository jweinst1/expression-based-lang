# expression-based-lang
A prototype for an expression based programming language

## Intro

Programming languages, whether they are high level or low level, are compiled to instruction level code. These instructions are usually called *byte* codes. Byte codes are executed on virtual machines. Traditionally, virtual machines follow a stack-based format or a register-base format. CPU's are almost always created to use registers for computations.

### Stack Machines

Stack machines follow a model where, instruction codes involve pushing and popping from a stack. This usually happens in a FILO fashion, such as

```
[top] <- [2nd][1st] (pop)
```

### Register Machines

Register based machines use a finite number of registers to facilitate computation. This is generally very diffcult to capture within a virtual machine. Lua has done this though, however. 

## Automata-based Machine

This prototype uses a state map, to implement more of a FSA type of virtual machine. It uses a system such as `<set val>`, as an instruction to set a value, than takes in a series of state changing instructions to alter that value. Instructions also handle nested values.

So an example is the expression `5 + (6 - 2)`, would be translated to something like


## Example

To build the example and run the integrated test, run the following command in your shell:

```
$ gcc exp_lang.c -o exp_lang
$ ./exp_lang
```

You will see the result:

```
eval lang test
evaluating nested integer sum, the result should be 16
result type is int, this passes.
the integer result is =  16
evaluating if else instruction test
result is int, pass so far
the integer result is =  13
```
