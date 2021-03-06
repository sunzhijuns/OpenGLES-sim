/* 
 * Copyright (c) 2013, Liou Jhe-Yu <lioujheyu@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
%{
#include "context.h"

#include "GPU/instruction_def.h"
#include "GPU/gpu_config.h"

int nvgp4ASM_lex(void);
void nvgp4ASM_error(char *s);
void nvgp4ASM_str_in(const char *s);

instruction t_inst;
operand t_operand;
std::vector<operand> operandPool;
std::vector<instruction> instructionPool;

extern programObject t_program;
extern unsigned int shaderType;
%}

%union {
	int		ival;
	float	fval;
	char	sval[30];
}

%token TEXTURE VERTEX FRAGMENT RESULT PROF END
%token ATTRIB POSITION RESULT_COLOR0
%token <ival> SHADERTYPE
%token <ival> INTERP_MODE
%token <sval> IDENTIFIER
%token <ival> INTEGER
%token <fval> FLOAT

%token <ival> VECTOROP SCALAROP BINSCOP VECSCAOP
%token <ival> BINOP TRIOP SWZOP TEXOP TXDOP
%token <ival> BRAOP FLOWCCOP IFOP REPOP ENDFLOWOP
%token <ival> KILOP DERIVEOP

%token <ival> OPMODIFIER
%token <ival> TEXTARGET
%token <ival> CCMASKRULE
%token <sval> XYZW_SWIZZLE RGBA_SWIZZLE
%token <ival> REG

%type <ival> opModifierItem interpModifier 
%type <ival> component xyzwComponent rgbaComponent swizzleSuffix

%type <fval> constantScalar
%type <ival> texImageUnit

%type <sval> optSign
%%

input
	:	line input
	|	/* empty */
	;

line:	profile
	|	namingStatement ';'
	|	instruction ';' {
			if (shaderType == 0)
				t_program.VSinstructionPool.push_back(t_inst);
			else
				t_program.FSinstructionPool.push_back(t_inst);
				
			t_inst.Init();
			t_operand.Init();
			operandPool.clear();
		};
	|	instLabel ':'
	|	END
	;
	
profile: PROF SHADERTYPE {shaderType = $2;}

namingStatement
	:	interpModifier ATTRIB_statement '{' primitive '.' ATTRIB '[' INTEGER '.' '.' INTEGER ']' '}' {
			for (int i= $8 + 1; i <= ($11+1); i++) {
				t_program.varyInterpMode[i] = $1;
			}
		};

interpModifier
	:	/* empty */	{$$ = 0;}
	|	interpModifier INTERP_MODE	{$$ = $1|$2;}
	;
	
ATTRIB_statement: ATTRIB IDENTIFIER '[' ']' '=' 

instruction
	:	ALUInstruction
	|	TexInstruction
	|	FlowInstruction
	|	SpecialInstrution
	;

ALUInstruction
	:	VECTORop_instruction
	|	SCALARop_instruction
	|	BINSCop_instruction
	|	BINop_instruction
	|	VECSCAop_instruction
	|	TRIop_instruction
	|	SWZop_instruction
	;

TexInstruction
	:	TEXop_instruction
	|	TXDop_instruction
	;

FlowInstruction
	:	BRAop_instruction
	|	FLOWCCop_instruction
	|	IFop_instruction
	|	REPop_instruction
	|	ENDFLOWop_instruction
	;
	
SpecialInstrution
	:	KILop_instruction
	|	DERIVEop_instruction
	;

VECTORop_instruction: VECTOROP opModifiers instResult ',' instOperand {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
	};

SCALARop_instruction: SCALAROP opModifiers instResult ',' instOperand {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
	};

BINSCop_instruction: BINSCOP opModifiers instResult ',' instOperand ',' instOperand {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
		t_inst.src[1] = operandPool[2];
	};

VECSCAop_instruction: VECSCAOP opModifiers instResult ',' instOperand ',' instOperand {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
		t_inst.src[1] = operandPool[2];
	};

BINop_instruction: BINOP opModifiers instResult ',' instOperand ',' instOperand {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
		t_inst.src[1] = operandPool[2];
	};

TRIop_instruction: TRIOP opModifiers instResult ',' instOperand ',' instOperand ',' instOperand {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
		t_inst.src[1] = operandPool[2];
		t_inst.src[2] = operandPool[3];
	};

SWZop_instruction: SWZOP opModifiers instResult ',' instOperand ',' extendedSwizzle {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
	};

TEXop_instruction: TEXOP opModifiers instResult ',' instOperand ',' texAccess {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
	};

TXDop_instruction: TXDOP opModifiers instResult ',' instOperand ',' instOperand ',' instOperand ',' texAccess {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
		t_inst.src[1] = operandPool[2];
		t_inst.src[2] = operandPool[3];
	};

BRAop_instruction: BRAOP opModifiers instTarget optBranchCond

FLOWCCop_instruction: FLOWCCOP opModifiers optBranchCond {
		
	};

IFop_instruction: IFOP opModifiers ccTest {
		t_inst.op = $1;
		t_inst.src[0] = t_operand;
		t_inst.src[0].type = INST_CCREG;
		switch (t_operand.ccMask) {
		case CC_EQ:
		case CC_EQ0:
		case CC_GE:
		case CC_GE0:
		case CC_GT:
		case CC_GT0:
		case CC_LE:
		case CC_LE0:
		case CC_LT:
		case CC_LT0:
		case CC_NE:
		case CC_NE0:
		case CC_TR:
		case CC_TR0:
		case CC_FL:
		case CC_FL0:
		case CC_NAN:
		case CC_NAN0:
		case CC_LEG:
		case CC_LEG0:
		case CC_CF:
		case CC_CF0:
		case CC_NCF:
		case CC_NCF0:
		case CC_OF:
		case CC_OF0:
		case CC_NOF:
		case CC_NOF0:
		case CC_AB:
		case CC_AB0:
		case CC_BLE:
		case CC_BLE0:
		case CC_SF:
		case CC_SF0:
		case CC_NSF:
		case CC_NSF0:
			t_inst.src[0].id = 0;
			break;
				
		case CC_EQ1:
		case CC_GE1:
		case CC_GT1:
		case CC_LE1:
		case CC_LT1:
		case CC_NE1:
		case CC_TR1:
		case CC_FL1:
		case CC_NAN1:
		case CC_LEG1:
		case CC_CF1:
		case CC_NCF1:
		case CC_OF1:
		case CC_NOF1:
		case CC_AB1:
		case CC_BLE1:
		case CC_SF1:
		case CC_NSF1:
			t_inst.src[0].id = 1;
			break;
		}
	}

REPop_instruction
	:	REPOP opModifiers instOperand {
			t_inst.op = $1;
			t_inst.src[0] = operandPool[0];
		};
	|	REPOP opModifiers {
			t_inst.op = $1;
		}
	;

ENDFLOWop_instruction: ENDFLOWOP opModifiers {
		t_inst.op = $1;
	}
	
KILop_instruction
	:	KILOP opModifiers ccTest {
			t_inst.op = $1;
			t_inst.src[0] = t_operand;
			t_inst.src[0].type = INST_CCREG;
			switch (t_operand.ccMask) {
			case CC_EQ:
			case CC_EQ0:
			case CC_GE:
			case CC_GE0:
			case CC_GT:
			case CC_GT0:
			case CC_LE:
			case CC_LE0:
			case CC_LT:
			case CC_LT0:
			case CC_NE:
			case CC_NE0:
			case CC_TR:
			case CC_TR0:
			case CC_FL:
			case CC_FL0:
			case CC_NAN:
			case CC_NAN0:
			case CC_LEG:
			case CC_LEG0:
			case CC_CF:
			case CC_CF0:
			case CC_NCF:
			case CC_NCF0:
			case CC_OF:
			case CC_OF0:
			case CC_NOF:
			case CC_NOF0:
			case CC_AB:
			case CC_AB0:
			case CC_BLE:
			case CC_BLE0:
			case CC_SF:
			case CC_SF0:
			case CC_NSF:
			case CC_NSF0:
				t_inst.src[0].id = 0;
				break;
					
			case CC_EQ1:
			case CC_GE1:
			case CC_GT1:
			case CC_LE1:
			case CC_LT1:
			case CC_NE1:
			case CC_TR1:
			case CC_FL1:
			case CC_NAN1:
			case CC_LEG1:
			case CC_CF1:
			case CC_NCF1:
			case CC_OF1:
			case CC_NOF1:
			case CC_AB1:
			case CC_BLE1:
			case CC_SF1:
			case CC_NSF1:
				t_inst.src[0].id = 1;
				break;
			}
		};
//	|	KILOP opModifiers instOperand {
//			t_inst.op = $1;
//			t_inst.src[0] = operandPool[0];
//		};
	;
	
DERIVEop_instruction: DERIVEOP opModifiers instResult ',' instOperand {
		t_inst.op = $1;
		t_inst.dst = operandPool[0];
		t_inst.src[0] = operandPool[1];
	}

opModifiers
	: 	/* empty */
	|	opModifierItem opModifiers
	;

opModifierItem: '.' OPMODIFIER	{t_inst.opModifiers[$2] = true;}

texAccess: texImageUnit ',' TEXTARGET {
		if (shaderType == 0) { // Vertex shader {
			//Use idx to record the array element if target is array type.
			int idx  = $1 - t_program.asmVStexIdx[$1].idx;
			t_inst.tid = t_program.srcTexture[t_program.asmVStexIdx[$1].name].idx + idx;
		}
		else {// Fragment shader
			int idx  = $1 - t_program.asmFStexIdx[$1].idx;
			t_inst.tid = t_program.srcTexture[t_program.asmFStexIdx[$1].name].idx + idx;
		}
		t_inst.tType = $3;
	};

texImageUnit: TEXTURE '[' INTEGER ']' {$$ = $3;}

optBranchCond
	:	/* empty */
	|	ccMask
	;

instOperand
	:	instOperandAbs	{operandPool.push_back(t_operand); t_operand.Init();}
	|	instOperandBase	{operandPool.push_back(t_operand); t_operand.Init();}
	;

instOperandAbs: optSign '|' instOperandBase '|' {t_operand.abs = true;}

instOperandBase
	:	optSign primitive '.' ATTRIB '[' INTEGER ']' swizzleSuffix {
			if (shaderType == VERTEX_SHADER)
				t_operand.id = $6;
			else // Fragment shader
				t_operand.id = t_program.srcVarying[t_program.asmFSinIdx[$6]].idx;
			t_operand.type = INST_ATTRIB;
			t_operand.modifier = $8;
			if ($1[0] == '-')
				t_operand.inverse = true;
		};
	|	optSign 'c' '[' INTEGER ']' swizzleSuffix {
			if (shaderType == 0) { // Vertex shader {
				//Use idx to record the array element if target is array type.
				int idx  = $4 - t_program.asmUniformVSIdx[$4].idx;
				t_operand.id = t_program.srcUniform[t_program.asmUniformVSIdx[$4].name].idx + idx;
			}
			else {// Fragment shader
				int idx  = $4 - t_program.asmUniformFSIdx[$4].idx;
				t_operand.id = t_program.srcUniform[t_program.asmUniformFSIdx[$4].name].idx + idx;
			}
			t_operand.type = INST_UNIFORM;
			t_operand.modifier = $6;
			if ($1[0] == '-')
				t_operand.inverse = true;
		};
	|	optSign REG swizzleSuffix {
			t_operand.id = $2;
			t_operand.type = INST_REG;
			t_operand.modifier = $3;
			if ($1[0] == '-')
				t_operand.inverse = true;
		};
	|	optSign constantVector swizzleSuffix {
			t_operand.type = INST_CONSTANT;
			t_operand.modifier = $3;
			if ($1[0] == '-')
				t_operand.inverse = true;
		};
	;

primitive
	:	FRAGMENT
	|	VERTEX
	;

instResult
	:	instResultCC
	|	instResultBase	{operandPool.push_back(t_operand);}
	;

instResultCC: instResultBase ccMask

instResultBase
	:	REG swizzleSuffix {
			t_operand.id = $1;
			t_operand.type = INST_REG;
			t_operand.modifier = $2;
		};
	|	RESULT '.' POSITION swizzleSuffix {
			t_operand.id = 0;
			t_operand.type = INST_ATTRIB;
			t_operand.modifier = $4;
		};
	|	RESULT '.' ATTRIB '[' INTEGER ']' swizzleSuffix {
			t_operand.id = $5 + 1;
			t_operand.type = INST_ATTRIB;
			t_operand.modifier = $7;
		};
	|	RESULT_COLOR0 swizzleSuffix {
			t_operand.type = INST_COLOR;
			t_operand.modifier = $2;
		};
	;

ccMask: '(' ccTest ')'

ccTest: CCMASKRULE swizzleSuffix {
		t_operand.ccMask = $1;
		t_operand.ccModifier = $2;
	}

constantVector: '{' constantVectorList '}'

constantVectorList
	:	constantScalar	{
			t_operand.val.x = t_operand.val.y =
			t_operand.val.z = t_operand.val.w = $1;
		};
	|	constantScalar ',' constantScalar {
			t_operand.val.x = $1;
			t_operand.val.y = t_operand.val.z = t_operand.val.w = $3;
		};
	|	constantScalar ',' constantScalar ',' constantScalar {
			t_operand.val.x = $1;
			t_operand.val.y = $3;
			t_operand.val.z = t_operand.val.w = $5;
		};
	|	constantScalar ',' constantScalar ',' constantScalar ',' constantScalar {
			t_operand.val.x = $1;
			t_operand.val.y = $3;
			t_operand.val.z = $5;
			t_operand.val.w = $7;
		};
	;

constantScalar
	:	INTEGER	{$$ = $1;}
	|	FLOAT	{$$ = $1;}
	;

swizzleSuffix
	:	/* empty */			{$$ = 0x8421;} // w=b1000(w) z=b0100(z) y=0010(y) x=0001(x)
	|	'.' component		{$$ = $2;}
	|	'.' XYZW_SWIZZLE	{
			$$ = 0;
			for (int i=0; i<4; i++) {
				if ($2[i] == '/0')
					break;
					
				switch ($2[i]) {
				case 'x':
					$$ = $$ | (0x1 << i*4);
					break;
				case 'y':
					$$ = $$ | (0x2 << i*4);
					break;
				case 'z':
					$$ = $$ | (0x4 << i*4);
					break;
				case 'w':
					$$ = $$ | (0x8 << i*4);
					break;
				}
			}
		}
	|	'.' RGBA_SWIZZLE	{
			$$ = 0;
			for (int i=0; i<4; i++) {
				if ($2[i] == '/0')
					break;
					
				switch ($2[i]) {
				case 'r':
					$$ = $$ | (0x1 << i*4);
					break;
				case 'g':
					$$ = $$ | (0x2 << i*4);
					break;
				case 'b':
					$$ = $$ | (0x4 << i*4);
					break;
				case 'a':
					$$ = $$ | (0x8 << i*4);
					break;
				}
			}
		}
	;

extendedSwizzle: extSwizComp ',' extSwizComp ',' extSwizComp ',' extSwizComp

extSwizComp
	:	optSign xyzwExtSwizSel
	|	optSign rgbaExtSwizSel
	;

xyzwExtSwizSel
	:	INTEGER
	|	xyzwComponent
	;

rgbaExtSwizSel: rgbaComponent

component
	:	xyzwComponent {$$ = $1;}
	|	rgbaComponent {$$ = $1;}
	;

xyzwComponent
	:	'x'	{$$ = 0x1;};
	|	'y' {$$ = 0x2;};
	|	'z' {$$ = 0x4;};
	|	'w' {$$ = 0x8;};
	;        
             
rgbaComponent
	:	'r' {$$ = 0x1;};
	|	'g' {$$ = 0x2;};
	|	'b' {$$ = 0x4;};
	|	'a' {$$ = 0x8;};
	;
	
optSign
	:	/* empty */	{$$[0] = '\0';};
	|	'-'			{$$[0] = '-'; $$[1] = '\0';};
	|	'+'			{$$[0] = '+'; $$[1] = '\0';};

instTarget: IDENTIFIER

instLabel: IDENTIFIER

%%

