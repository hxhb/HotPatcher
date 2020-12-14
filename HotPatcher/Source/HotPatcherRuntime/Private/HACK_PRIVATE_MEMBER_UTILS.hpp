// #include <iostream>
// using namespace std;
namespace Hacker
{
	template<typename Tag, typename Tag::MemType M>
	struct PrivateMemberStealer
	{
		// define friend funtion GetPrivate,return class member pointer
		friend typename Tag::MemType GetPrivate(Tag) { return M; }
	};
}
#define DECL_HACK_PRIVATE_DATA(ClassName,DataType,DataName) namespace Hacker{\
                                                              struct ClassName##_##DataName\
                                                              {\
                                                                typedef DataType ClassName::*MemType;\
                                                                friend MemType GetPrivate(ClassName##_##DataName);\
                                                              };\
                                                              template struct PrivateMemberStealer<ClassName##_##DataName, &ClassName::DataName>;\
                                                            }
#define GET_VAR_PRIVATE_DATA_MEMBER(ClassInstancePointer,ClassName,DataName)	ClassInstancePointer->*GetPrivate(::Hacker::ClassName##_##DataName())
#define GET_REF_PRIVATE_DATA_MEMBER(RefName,ClassInstancePointer,ClassName,DataName)	auto& RefName = ClassInstancePointer->*GetPrivate(::Hacker::ClassName##_##DataName())


#define DECL_HACK_PRIVATE_NOCONST_FUNCTION(ClassName,MemberName,ReturnType,...) \
		namespace Hacker{\
			struct ClassName##_##MemberName \
			{\
				typedef ReturnType (ClassName::*MemType)(__VA_ARGS__);\
				friend MemType GetPrivate(ClassName##_##MemberName);\
			};\
			template struct PrivateMemberStealer<ClassName##_##MemberName, &ClassName::MemberName>;\
		}
#define DECL_HACK_PRIVATE_CONST_FUNCTION(ClassName,MemberName,ReturnType,...) \
		namespace Hacker{\
			struct ClassName##_##MemberName \
			{\
				typedef ReturnType (ClassName::*MemType)(__VA_ARGS__)const;\
				friend MemType GetPrivate(ClassName##_##MemberName);\
			};\
			template struct PrivateMemberStealer<ClassName##_##MemberName, &ClassName::MemberName>;\
		}

// using ADL found to ::Hacker::Getprivate
#define GET_PRIVATE_MEMBER_FUNCTION(ClassName,MemberName) GetPrivate(::Hacker::ClassName##_##MemberName())
#define CALL_MEMBER_FUNCTION(ClassPointer,MemberFuncPointer,...) (ClassPointer->*MemberFuncPointer)(__VA_ARGS__)

// class A{
// public:
// 	A(int ivalp=0):ival{ivalp}{}
// private:
// 	int func(int ival)const
// 	{
// 		printf("A::func(int)\t%d\n",ival);
// 		printf("ival is %d\n",ival);
// 		return 0;
// 	}
// 	int ival;
// };

// DECL_HACK_PRIVATE_DATA(A,int,ival)
// DECL_HACK_PRIVATE_CONST_FUNCTION(A, func, int, int)

// int main()
// {
// 	A aobj(789);
// 	// get private data member
// 	GET_REF_PRIVATE_DATA_MEMBER(ref_ival, &aobj, A, ival);
// 	ref_ival=456;
// 	cout<<GET_VAR_PRIVATE_DATA_MEMBER(&aobj, A, ival)<<endl;
// 	// // call private func
// 	auto A_func=GET_PRIVATE_MEMBER_FUNCTION(A, func);
// 	cout<<CALL_MEMBER_FUNCTION(&aobj, A_func, 123);
// }