using namespace std;
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

#define DECL_HACK_PRIVATE_STATIC_DATA(ClassName,DataType,DataName) namespace Hacker{\
                                                              struct ClassName##_##DataName\
                                                              {\
                                                                typedef DataType* MemType;\
                                                                friend MemType GetPrivate(ClassName##_##DataName);\
                                                              };\
                                                              template struct PrivateMemberStealer<ClassName##_##DataName, &ClassName::DataName>;\
                                                            }

#define DECL_HACK_PRIVATE_STATIC_FUNCTION(ClassName,MemberName,ReturnType,...) \
		namespace Hacker{\
			struct ClassName##_##MemberName \
			{\
				typedef ReturnType (*MemType)(__VA_ARGS__);\
				friend MemType GetPrivate(ClassName##_##MemberName);\
			};\
			template struct PrivateMemberStealer<ClassName##_##MemberName, &ClassName::MemberName>;\
		}

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

#define GET_VAR_PRIVATE_DATA_MEMBER(ClassInstancePointer,ClassName,DataName)	ClassInstancePointer->*GetPrivate(::Hacker::ClassName##_##DataName())
#define GET_REF_PRIVATE_DATA_MEMBER(RefName,ClassInstancePointer,ClassName,DataName)	auto& RefName = ClassInstancePointer->*GetPrivate(::Hacker::ClassName##_##DataName())
#define GET_PRIVATE_STATIC_DATA_MEMBER_PTR(PtrName,ClassName,DataName)	auto PtrName = GetPrivate(::Hacker::ClassName##_##DataName())
// using ADL found to ::Hacker::Getprivate
#define GET_PRIVATE_MEMBER_FUNCTION(ClassName,MemberName) GetPrivate(::Hacker::ClassName##_##MemberName())
#define CALL_MEMBER_FUNCTION(ClassPointer,MemberFuncPointer,...) (ClassPointer->*MemberFuncPointer)(__VA_ARGS__)

// class A{
// public:
// 	A(int ivalp=0):ival{ivalp}{}
//
// 	static void printStaticIval2()
// 	{
// 		printf("%d\n",A::static_ival2);
// 	}
// private:
// 	int func(int ival)const
// 	{
// 		printf("A::func(int)\t%d\n",ival);
// 		printf("ival is %d\n",ival);
// 		return 0;
// 	}
// 	int noconst_func(int ival)
// 	{
// 		printf("A::noconst_func(int)\t%d\n",ival);
// 		printf("ival is %d\n",ival);
// 		return 0;
// 	}
// 	int ival;
//
// 	static int static_ival2;
//
// };
//
// // private member data 
// DECL_HACK_PRIVATE_DATA(A,int,ival)
// // private member const function
// DECL_HACK_PRIVATE_CONST_FUNCTION(A, func, int, int)
// // private member not-const function
// DECL_HACK_PRIVATE_NOCONST_FUNCTION(A,noconst_func,int,int)
// // private static member
// DECL_HACK_PRIVATE_STATIC_DATA(A,int,static_ival2)
// // private static member function
// DECL_HACK_PRIVATE_STATIC_FUNCTION(A,printStaticIval2,void);
//
// int A::static_ival2 = 666;
//
// int main()
// {
// 	A aobj(789);
// 	// get private non-static data member
// 	GET_REF_PRIVATE_DATA_MEMBER(ref_ival, &aobj, A, ival);
// 	ref_ival=456;
// 	std::cout<<GET_VAR_PRIVATE_DATA_MEMBER(&aobj, A, ival)<<endl;
//
// 	// call private non-static member function
// 	auto A_func=GET_PRIVATE_MEMBER_FUNCTION(A, func);
// 	std::cout<<CALL_MEMBER_FUNCTION(&aobj, A_func, 123)<<std::endl;
//
// 	// get private static data member
// 	GET_PRIVATE_STATIC_DATA_MEMBER_PTR(A_static_ival2, A, static_ival2);
// 	std::cout<<*A_static_ival2<<std::endl;
// 	*A_static_ival2 = 123456;
//
// 	// cll private static function member
// 	auto A_printStaticIval2_func_ptr = GET_PRIVATE_MEMBER_FUNCTION(A,printStaticIval2);
// 	A_printStaticIval2_func_ptr();
//
// }