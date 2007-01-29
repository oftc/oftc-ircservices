use ExtUtils::MakeMaker;
WriteMakefile(
    NAME              => 'Services',
    VERSION_FROM      => 'Services.pm', 
    ($] >= 5.005 ?     
      (ABSTRACT_FROM  => 'Services.pm', 
       AUTHOR         => 'Stuart Walsh <stu@ipng.org.uk>') : ()),
    INC               => '-I.', 
);
