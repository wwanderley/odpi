//-----------------------------------------------------------------------------
// Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
// This program is free software: you can modify it and/or redistribute it
// under the terms of:
//
// (i)  the Universal Permissive License v 1.0 or at your option, any
//      later version (http://oss.oracle.com/licenses/upl); and/or
//
// (ii) the Apache License v 2.0. (http://www.apache.org/licenses/LICENSE-2.0)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// DemoBindObjectsLeak.c
//   Find leak binding objects.
//
/*
CREATE OR REPLACE PACKAGE test_pkg_types AS	
	TYPE my_record IS RECORD (num NUMBER(5), tex VARCHAR2(1000));	
	TYPE my_table IS TABLE OF my_record;	
END test_pkg_types;

CREATE OR REPLACE PACKAGE test_pkg_sample AS		
	FUNCTION test_table (
		x NUMBER
	) RETURN test_pkg_types.my_table;	
END test_pkg_sample;

CREATE OR REPLACE PACKAGE BODY test_pkg_sample AS
	FUNCTION test_table (
		x NUMBER
	) RETURN test_pkg_types.my_table IS
		tb     test_pkg_types.my_table;
		item   test_pkg_types.my_record;
	BEGIN
		tb := test_pkg_types.my_table();
		FOR c IN (
			SELECT
				level "LEV"
			FROM
				"SYS"."DUAL" "A1"
			CONNECT BY
				level <= x
		) LOOP
			item.num := c.lev;
			item.tex := 'test - ' || ( c.lev * 2 );			
			tb.extend();
			tb(tb.count) := item;
		END LOOP;
	
		RETURN tb;
	END test_table;	
END test_pkg_sample;
*/
//-----------------------------------------------------------------------------
/*
 export ODPIC_SAMPLES_CONNECT_STRING=localhost:1521/xe
 export ODPIC_SAMPLES_MAIN_USER=System
 export ODPIC_SAMPLES_MAIN_PASSWORD=Oracle18
 DPI_DEBUG_LEVEL=38 LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../lib build/DemoBindObjectsLeak
*/


#include "SampleLib.h"
#define OBJECT_TYPE_NAME    "TEST_PKG_TYPES.MY_TABLE"
#define ELEMENT_OBJECT_TYPE_NAME    "TEST_PKG_TYPES.MY_RECORD"
#define SQL_TEXT            "begin :1 := TEST_PKG_SAMPLE.TEST_TABLE('2'); end;"
#define NUM_ATTRS           1
#define ELEMENT_NUM_ATTRS   2

//-----------------------------------------------------------------------------
// main()
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{    
    dpiObjectAttr *attrs[NUM_ATTRS], *elementAttrs[ELEMENT_NUM_ATTRS];
    int32_t elementIndex, nextElementIndex;
    int i, exists;
    dpiObjectType *objType, *elementObjType;
    dpiObject *obj, *elementObj;
    dpiStmt *stmt;
    dpiConn *conn;
    dpiData *objectValue, elementValue, numberValue, strValue;
    dpiVar *objectVar;
    
    // connect to database
    conn = dpiSamples_getConn(0, NULL);

    // get object type and attributes
    if (dpiConn_getObjectType(conn, OBJECT_TYPE_NAME, strlen(OBJECT_TYPE_NAME),
            &objType) < 0)
        return dpiSamples_showError();
    if (dpiObjectType_getAttributes(objType, NUM_ATTRS, attrs) < 0)
        return dpiSamples_showError();

    if (dpiConn_getObjectType(conn, ELEMENT_OBJECT_TYPE_NAME, strlen(ELEMENT_OBJECT_TYPE_NAME),
            &elementObjType) < 0)
        return dpiSamples_showError();
    if (dpiObjectType_getAttributes(elementObjType, ELEMENT_NUM_ATTRS, elementAttrs) < 0)
        return dpiSamples_showError();    
    
    if (dpiObjectType_createObject(objType, &obj) < 0)
        return dpiSamples_showError();

    // prepare and execute statement
    if (dpiConn_newVar(conn, DPI_ORACLE_TYPE_OBJECT, DPI_NATIVE_TYPE_OBJECT, 1,
            0, 0, 0, objType, &objectVar, &objectValue) < 0)
        return dpiSamples_showError();

    if (dpiConn_prepareStmt(conn, 0, SQL_TEXT, strlen(SQL_TEXT), NULL, 0,
            &stmt) < 0)
        return dpiSamples_showError();

    if (dpiStmt_bindByPos(stmt, 1, objectVar) < 0)
        return dpiSamples_showError();

    if (dpiStmt_execute(stmt, 0, NULL) < 0)
        return dpiSamples_showError();        

    if(dpiObject_addRef(objectValue->value.asObject) < 0)
            return dpiSamples_showError();

    if (dpiObject_getFirstIndex(objectValue->value.asObject, &elementIndex, &exists) < 0)
        return dpiSamples_showError();

    while (1) {
        if (dpiObject_getElementValueByIndex(objectValue->value.asObject,
                elementIndex, DPI_NATIVE_TYPE_OBJECT, &elementValue) < 0)
            return dpiSamples_showError();        

        elementObj = elementValue.value.asObject;

        if(dpiObject_addRef(elementObj) < 0)
            return dpiSamples_showError();        

        if (dpiObject_getAttributeValue(elementObj, elementAttrs[0], DPI_NATIVE_TYPE_INT64,
            &numberValue) < 0)
            return dpiSamples_showError();    

        if (dpiObject_getAttributeValue(elementObj, elementAttrs[1], DPI_NATIVE_TYPE_BYTES,
            &strValue) < 0)
            return dpiSamples_showError();        

        printf("    [%d]: %ld, %.*s\n", elementIndex, numberValue.value.asInt64,
                strValue.value.asBytes.length,
                strValue.value.asBytes.ptr);

        dpiObject_release(elementObj);        

        if (dpiObject_getNextIndex(objectValue->value.asObject, elementIndex,
                &nextElementIndex, &exists) < 0)
            return dpiSamples_showError();
        if (!exists)
            break;
        elementIndex = nextElementIndex;
    }         

    printf("Clean up...\n");
    dpiObject_release(objectValue->value.asObject);
    dpiObject_release(obj);    

    dpiVar_release(objectVar);    
    dpiStmt_release(stmt);

    // close Object Type
    for (i = 0; i < ELEMENT_NUM_ATTRS; i++)
        dpiObjectAttr_release(elementAttrs[i]);    

    dpiObjectType_release(elementObjType);    

    for (i = 0; i < NUM_ATTRS; i++)
        dpiObjectAttr_release(attrs[i]);    
    
    dpiObjectType_release(objType);        

    dpiConn_release(conn);

    printf("Done.\n");
    return 0;
}
